#include "Swapchain.hpp"
#include "engine/vulkan/Image.hpp"
#include "engine/vulkan/Window.hpp"

Swapchain::Swapchain(DeviceSettings& deviceSettings){
    deviceSettings.extensions.push_back(vk::KHRSwapchainExtensionName);
}
Swapchain::~Swapchain(){

}

void Swapchain::create(Window& window,Device& device)
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = device.physicalDevice.getSurfaceCapabilitiesKHR( *window.surface );
    swapChainExtent                                = chooseSwapExtent(window,surfaceCapabilities);
    uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = device.physicalDevice.getSurfaceFormatsKHR( window.surface );
    surfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = device.physicalDevice.getSurfacePresentModesKHR( window.surface );

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface          = *window.surface,
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
    swapChain       = vk::raii::SwapchainKHR( device.device, swapChainCreateInfo );
    auto _images = swapChain.getImages();
    {
        images.clear();
        for (auto &_image : _images)
        {
            Image::Reincarnation image;
            image.initExisitingImage(_image,swapChainExtent);
            image.createImageView(
                device, 
                surfaceFormat.format, 
                vk::ImageAspectFlagBits::eColor);
            images.emplace_back( std::move(image) );
        }
    }   
}
void Swapchain::recreate(Window& window,Device& device){
    device.device.waitIdle();

    // cleanup swap chain
    swapChain = nullptr;
    create(window,device);
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
    



