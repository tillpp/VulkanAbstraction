#pragma once
#include "engine/vulkan/common.hpp" // IWYU pragma: keep
#include "engine/vulkan/Instance.hpp"
#include "engine/vulkan/DeviceFeatures.hpp"
#include <functional>


struct DeviceSettings{
    std::vector<const char*> extensions;
    std::vector<class Queue*> queues;

    std::vector<std::function<void(class Device&)>> callAfterCreation;
};

class Device
{
public:
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;

    void create(Instance& instance,DeviceSettings settings,const DeviceFeatures& features);
private:
    std::optional<int> isDeviceSuitable( vk::raii::PhysicalDevice const & physicalDevice ,DeviceSettings settings,const DeviceFeatures& features);
    vk::raii::PhysicalDevice pickPhysicalDevice(Instance& instance,const DeviceSettings& settings,const DeviceFeatures& features);
    void initLogicalDevice(const DeviceSettings& settings,const DeviceFeatures& features);
    
};
