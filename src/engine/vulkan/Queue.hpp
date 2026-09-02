#pragma once
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/common.hpp" // IWYU pragma: keep

class Queue:public vk::raii::Queue
{
public:
    Queue();

    void create(DeviceSetup& deviceSetup);

    uint32_t queueFamilyIndex;
    float    priority = 0.5f;
    virtual bool isQueueFamilySuitable(vk::QueueFamilyProperties const & qfp,size_t queueFamilyIndex, vk::raii::PhysicalDevice& physicalDevice)const=0;
    
};