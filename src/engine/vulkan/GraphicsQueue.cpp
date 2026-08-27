#include "GraphicsQueue.hpp"
#include "engine/vulkan/Window.hpp"

void GraphicsQueue::create(Window& window,DeviceSettings& deviceSettings){
    Queue::create(deviceSettings);
    this->window = &window;
}

bool GraphicsQueue::isQueueFamilySuitable(vk::QueueFamilyProperties const & qfp, size_t queueFamilyIndex, vk::raii::PhysicalDevice& physicalDevice){
    return (qfp.queueFlags & vk::QueueFlagBits::eGraphics)&& (qfp.queueFlags & vk::QueueFlagBits::eTransfer) && physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, *window->surface);
}