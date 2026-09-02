#pragma once
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/common.hpp" // IWYU pragma: keep
#include "engine/vulkan/Queue.hpp"

class GraphicsQueue:public Queue{
    class Window* window = nullptr;

    using Queue::create;
public:
    void create(DeviceSetup& deviceSetup,class Window& window);
    virtual bool isQueueFamilySuitable(vk::QueueFamilyProperties const & qfp, size_t queueFamilyIndex, vk::raii::PhysicalDevice& physicalDevice)const override;   
};