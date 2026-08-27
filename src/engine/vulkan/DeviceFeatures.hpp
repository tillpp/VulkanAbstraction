#pragma once
#include "engine/vulkan/common.hpp" // IWYU pragma: keep


class DeviceFeatures
{
public:
    std::function<bool(const vk::raii::PhysicalDevice& physicalDevice)> physicalDeviceSuitable;
    void* logicalDeviceFeatures;
    DeviceFeatures(void* logicalDeviceFeatures, std::function<bool(const vk::raii::PhysicalDevice& physicalDevice)> physicalDeviceSuitable);
};
