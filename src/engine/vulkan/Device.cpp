#include "Device.hpp"
#include "Queue.hpp"
#include <vector>

void Device::create(Instance& instance,PhyDeviceFeatures& phyFeatures,void* logFeatures,DeviceSetup settings){
    this->phyFeatures = &phyFeatures;
    this->logFeatures = logFeatures;
    this->settings    = settings;
    this->instance    = &instance;

    physicalDevice = pickPhysicalDevice();
    initLogicalDevice();

    for(auto& callback:settings.callAfterCreation){
        callback->afterDeviceInit(*this);
    }
}

std::optional<int> Device::isDeviceSuitable( vk::raii::PhysicalDevice const & physicalDevice){
    auto deviceProperties = physicalDevice.getProperties();
    auto deviceFeatures = physicalDevice.getFeatures();
    uint32_t score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) {
        return 0; // worst possible alternative
    }else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 1e9;
    }else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
        score += 1e3;
    }


    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader)
    {
        return {};
    }
    // Check if the physicalDevice supports the Vulkan 1.3 API version
    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
    
    // Check if any of the queue families support graphics operations
    auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
    bool supportsGraphics = std::ranges::any_of( queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } );

    // Check if all required physicalDevice extensions are available
    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtensions =
    std::ranges::all_of( settings.extensions,
        [&availableDeviceExtensions]( auto const & requiredDeviceExtension )
        {
        return std::ranges::any_of( availableDeviceExtensions,
                                    [requiredDeviceExtension]( auto const & availableDeviceExtension )
                                    { return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension ) == 0; } );
        } );

    bool supportsRequiredFeatures = phyFeatures->physicalDeviceSuitable(physicalDevice);

    // Return true if the physicalDevice meets all the criteria
    if(supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures)
        return score;
    return {};
}
vk::raii::PhysicalDevice Device::pickPhysicalDevice(){
    auto physicalDevices = vk::raii::PhysicalDevices( *instance );
    if (physicalDevices.empty())
    {
        throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
    }

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, vk::raii::PhysicalDevice> candidates;
    for (const auto& pd : physicalDevices)
    {
        auto score = isDeviceSuitable(pd);
        if(score.has_value())   
            candidates.insert(std::make_pair(score.value(), pd));
    }

    // Check if the best candidate is suitable at all
    if (!candidates.empty() && candidates.rbegin()->first > 0)
    {
        return candidates.rbegin()->second;
    }
    else
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}
std::optional<QueueFamilyIndex>       Device::findSuitableQueueFamily(const Queue* queue, vk::raii::PhysicalDevice& physicalDevice){
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    int32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if (queue->isQueueFamilySuitable(queueFamilyProperties[qfpIndex],qfpIndex,physicalDevice))
        {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        return {};
    }
    return queueIndex;
}
std::map<QueueFamilyIndex, std::vector<class Queue*>>  Device::bucketQueues(){
    /*
        sort the Queues after their QueueFamilyIndex they going to use.
    */
    std::map<QueueFamilyIndex,std::vector<Queue*>>  buckets; 
    for (auto &&queue : settings.queues){
        
        auto queueFamilyIndex = findSuitableQueueFamily(queue, physicalDevice);
        if(!queueFamilyIndex.has_value())
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        else 
            queue->queueFamilyIndex = queueFamilyIndex.value();
        
        buckets[queueFamilyIndex.value()].push_back(queue);
    }
    return buckets;
}
void Device::initLogicalDevice(){
    assert(settings.queues.size());
    auto buckets = bucketQueues();

    // turn Queues into vk::DeviceQueueCreateInfo  
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;

    std::vector<float> priorities;
    for(auto&& pair: buckets){
        std::ranges::transform(pair.second,std::back_inserter(priorities), &Queue::priority);
    }
    size_t indexInPriorities = 0;
    for(auto&& pair: buckets){
        auto familyIndex = pair.first;
        auto size = pair.second.size();
        deviceQueueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ 
            .queueFamilyIndex = familyIndex,
            .queueCount = (unsigned int)size,
            .pQueuePriorities = priorities.data()+indexInPriorities
        });
        indexInPriorities += size;
    }

    // create the device and all the queues.
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = logFeatures,
        .queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
        .pQueueCreateInfos = deviceQueueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(settings.extensions.size()),
        .ppEnabledExtensionNames = settings.extensions.data()
    }; 
    device = vk::raii::Device( physicalDevice, deviceCreateInfo );

    // init all the queues
    for(auto&& pair: buckets){
        auto familyIndex = pair.first;
        for (size_t i = 0; i < pair.second.size(); i++)
        {
            (vk::raii::Queue&)(*pair.second[i]) = vk::raii::Queue( device, familyIndex, i );
        }
        
    }
}
