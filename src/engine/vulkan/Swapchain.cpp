#include "engine/vulkan/Image.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"

void Swapchain::addRequirements(DeviceSetup& ds){
    ds.extensions.push_back(vk::KHRSwapchainExtensionName);
}
void Swapchain::create(Device& device,Window& window){
    this->window = &window;
    this->device = &device;

    createSwapchain();
    createSync();
}
void Swapchain::createSwapchain(){
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = device->physicalDevice.getSurfaceCapabilitiesKHR( *window->surface );
    swapChainExtent                                = chooseSwapExtent(*window,surfaceCapabilities);
    uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

        
    std::vector<vk::SurfaceFormatKHR> availableFormats = device->physicalDevice.getSurfaceFormatsKHR( window->surface );
    surfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = device->physicalDevice.getSurfacePresentModesKHR( window->surface );

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface          = *window->surface,
        .minImageCount    = minImageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = chooseSwapPresentMode(availablePresentModes),
        .clipped          = true,
    };
    swapChain       = vk::raii::SwapchainKHR( device->device, swapChainCreateInfo );
    auto _images = swapChain.getImages();

    {
        images.clear();
        for (auto &_image : _images)
        {
            Image::Reincarnation image;
            image.initExisitingImage(_image,swapChainExtent);
            image.createImageView(
                *device, 
                surfaceFormat.format, 
                vk::ImageAspectFlagBits::eColor);
            images.emplace_back( std::move(image) );
        }
    }  
    //TODO: update all dependees of the window screensize
    window->depthBuffer.recreate(*window,swapChainExtent);
}
void Swapchain::createSync(){
    // rendering stuff
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());
    for (size_t i = 0; i < images.size(); i++)
    {
        renderFinishedSemaphores.emplace_back(device->device, vk::SemaphoreCreateInfo());
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        presentCompleteSemaphores.emplace_back(device->device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(device->device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        commandBuffers.emplace_back(window->commandPool);
    }
}


// choose surface settings
vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}
vk::PresentModeKHR Swapchain::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes){
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
    return std::ranges::any_of(availablePresentModes,
                            [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
            vk::PresentModeKHR::eMailbox :
            vk::PresentModeKHR::eFifo;
}
vk::Extent2D Swapchain::chooseSwapExtent(Window& window,vk::SurfaceCapabilitiesKHR const &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}
uint32_t Swapchain::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}


// render sync
CommandBuffer& Swapchain::getCommandBuffer(){
    return commandBuffers[frameIndex];
}

uint32_t const& Swapchain::getFrameIndex()const{
    return frameIndex;
}
void Swapchain::recreate(){
    int width = 0, height = 0;
    glfwGetFramebufferSize(*window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(*window, &width, &height);
        glfwWaitEvents();
    }
    device->device.waitIdle();

    // cleanup swap chain
    swapChain = nullptr;
    createSwapchain();
}
Swapchain::Swapchain():trashCan(*this){
}
Swapchain::~Swapchain(){
    clear();
}
void Swapchain::clear(){
    if(device)
        device->device.waitIdle();
    trashCan.clearAll();
    commandBuffers.clear();
    presentCompleteSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();
    frameIndex = 0;
    device = nullptr;
    swapChain = nullptr;

}

bool Swapchain::begin(){
    auto& queue = window->gQueue;

    auto fenceResult = device->device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
	if (fenceResult != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence!");
	}
    auto [result, _imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
    if(result == vk::Result::eErrorOutOfDateKHR){
        recreate();
        return false;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    
    device->device.resetFences(*inFlightFences[frameIndex]);
	commandBuffers[frameIndex].commandBuffer.reset();

    imageIndex = _imageIndex;
    trashCan.clear();
    return true;
}
void Swapchain::end(){
    auto& queue = window->gQueue;
    
    vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    const vk::SubmitInfo   submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffers[frameIndex].commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]
    };
    queue.submit(submitInfo, *inFlightFences[frameIndex]);


    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapChain,
        .pImageIndices      = &imageIndex};
    auto result = queue.presentKHR(presentInfoKHR);
    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR)||window->shouldRecreateSwapchain){
        window->shouldRecreateSwapchain = false;
        recreate();
    }
    else
    {
        // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        assert(result == vk::Result::eSuccess);
    }
    
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}