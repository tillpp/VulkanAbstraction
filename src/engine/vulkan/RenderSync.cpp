
#include "RenderSync.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/DepthBuffer.hpp"
#include "engine/vulkan/Window.hpp"



void RenderSync::create(CommandPool& pool,Swapchain& swapchain){
    auto& device = pool.getDevice();
    this->device = &device;
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());
    for (size_t i = 0; i < swapchain.images.size(); i++)
    {
        renderFinishedSemaphores.emplace_back(device.device, vk::SemaphoreCreateInfo());
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        presentCompleteSemaphores.emplace_back(device.device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(device.device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        commandBuffers.emplace_back(pool);
    }
    
}
void RenderSync::TrashLayer::clear(){
    pipelineLayouts.clear();
    pipelines.clear();
    descriptorSets.clear();
    descriptorPools.clear();
    buffers.clear();
    deviceMemorys.clear();
    reincarnations.clear();
}

RenderSync::~RenderSync(){
    clear();
}
void RenderSync::clear(){
    if(device)
        device->device.waitIdle();
    for (auto& tl : trashLayer) {
        tl.clear();
    }
    commandBuffers.clear();
    presentCompleteSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();
    frameIndex = 0;
    device = nullptr;
}

uint32_t RenderSync::getFrameIndex()const{
    return frameIndex;
}
CommandBuffer& RenderSync::getCommandBuffer(){
    return commandBuffers[frameIndex];
}

void RenderSync::recreateSwapChain(Window& window,DepthBuffer* depthBuffer){
    int width = 0, height = 0;
    auto& device = window.commandPool.getDevice();
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    device.device.waitIdle();

    window.swapChain.recreate(window,device);
    if(depthBuffer)
        depthBuffer->recreate(window);
}

bool RenderSync::begin(
        Window& window,
        DepthBuffer* depthBuffer){

    auto& queue = window.commandPool.getQueue();
    auto& device = window.commandPool.getDevice();

    auto fenceResult = device.device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
	if (fenceResult != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence!");
	}
    auto [result, _imageIndex] = window.swapChain.swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
    if(result == vk::Result::eErrorOutOfDateKHR){
        
        recreateSwapChain(window,depthBuffer);
        return false;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    
    device.device.resetFences(*inFlightFences[frameIndex]);
	commandBuffers[frameIndex].commandBuffer.reset();

    imageIndex = _imageIndex;
    trashLayer[frameIndex].clear();
    return true;
}
void RenderSync::end(
    Window& window,
    DepthBuffer* depthBuffer){
    
    auto& queue = window.commandPool.getQueue();
    auto& device = window.commandPool.getDevice();

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
        .pSwapchains        = &*window.swapChain.swapChain,
        .pImageIndices      = &imageIndex};
    auto result = queue.presentKHR(presentInfoKHR);
    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR)||window.shouldRecreateSwapchain){
        window.shouldRecreateSwapchain = false;
        recreateSwapChain(window,depthBuffer);
    }
    else
    {
        // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        assert(result == vk::Result::eSuccess);
    }
    
    
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}
void RenderSync::trash(vk::raii::Buffer buffer,vk::raii::DeviceMemory deviceMemory){
    trashLayer[getFrameIndex()].buffers.push_back(std::move(buffer));
    trashLayer[getFrameIndex()].deviceMemorys.push_back(std::move(deviceMemory));
}
void RenderSync::trash(vk::raii::PipelineLayout pipelineLayout,vk::raii::Pipeline pipeline){
    trashLayer[getFrameIndex()].pipelineLayouts.push_back(std::move(pipelineLayout));
    trashLayer[getFrameIndex()].pipelines.push_back(std::move(pipeline));
}
void RenderSync::trash(vk::raii::DescriptorPool descriptorPool){
    trashLayer[getFrameIndex()].descriptorPools.push_back(std::move(descriptorPool));
}
void RenderSync::trash(vk::raii::DescriptorSet descriptorSet){
    trashLayer[getFrameIndex()].descriptorSets.push_back(std::move(descriptorSet));
}
void RenderSync::trash(std::shared_ptr<class ResourceReincarnation> reincarnation){
    trashLayer[getFrameIndex()].reincarnations.push_back(reincarnation);
}