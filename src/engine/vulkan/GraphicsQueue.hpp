#pragma once 
#include "engine/vulkan/common.hpp" // IWYU pragma: keep
#include "engine/vulkan/Queue.hpp"

class GraphicsQueue:public Queue
{
    class Window* window;
    using Queue::create;
public:
    void create(class Window& window,DeviceSettings& deviceSettings);
    virtual bool isQueueFamilySuitable(vk::QueueFamilyProperties const & qfp, size_t queueFamilyIndex, vk::raii::PhysicalDevice& physicalDevice) override;   
};