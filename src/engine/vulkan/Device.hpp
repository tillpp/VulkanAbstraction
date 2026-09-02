#pragma once
#include "engine/vulkan/Instance.hpp"
#include "engine/vulkan/common.hpp" // IWYU pragma: keep
#include <cstdint>
#include <map>

struct DeviceCallback{
    virtual void afterDeviceInit(class Device&)=0;
};
struct DeviceSetup{
    std::vector<const char*>  extensions;
    std::vector<class Queue*> queues;

    std::vector<DeviceCallback*> callAfterCreation;
};
struct PhyDeviceFeatures{
    virtual bool physicalDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)=0;
};
typedef uint32_t QueueFamilyIndex;
class Device{
    PhyDeviceFeatures*        phyFeatures = nullptr;
    void*                     logFeatures = nullptr;
    DeviceSetup               settings;
    Instance*                 instance = nullptr;
public:
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device         device         = nullptr;

    void                     create            (Instance& instance,PhyDeviceFeatures& phyfeatures,void* logicalDeviceFeatures,DeviceSetup settings);
private: //helper physical device
    std::optional<int>       isDeviceSuitable  (vk::raii::PhysicalDevice const & physicalDevice);
    vk::raii::PhysicalDevice pickPhysicalDevice();
private: //helper logical  device
    void                            initLogicalDevice ();
    std::optional<QueueFamilyIndex> findSuitableQueueFamily(const class Queue* queue, vk::raii::PhysicalDevice& physicalDevice);
    std::map<QueueFamilyIndex,
        std::vector<class Queue*>>  bucketQueues();
};