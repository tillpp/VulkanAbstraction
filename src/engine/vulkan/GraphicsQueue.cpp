#include "engine/vulkan/GraphicsQueue.hpp"
#include "Window.hpp"
#include <cassert>

void GraphicsQueue::create(DeviceSetup& deviceSetup,class Window& window){
    assert(!this->window);
    this->window = &window;
    Queue::create(deviceSetup);
}

bool GraphicsQueue::isQueueFamilySuitable(vk::QueueFamilyProperties const & qfp, size_t queueFamilyIndex, vk::raii::PhysicalDevice& physicalDevice)const{
    assert(window);
    return (qfp.queueFlags & vk::QueueFlagBits::eGraphics)&& (qfp.queueFlags & vk::QueueFlagBits::eTransfer) && physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, *window->surface);
}
