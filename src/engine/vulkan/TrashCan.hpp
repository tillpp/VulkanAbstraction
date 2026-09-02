#pragma once
#include "common.hpp"
#include <cstdint>

class TrashCan{
    uint32_t const& frameIndex;
public:
    struct TrashLayer{
        std::vector<vk::raii::Buffer> buffers;
        std::vector<vk::raii::DeviceMemory> deviceMemorys;
        std::vector<vk::raii::Pipeline> pipelines;
        std::vector<vk::raii::PipelineLayout> pipelineLayouts;        
        std::vector<vk::raii::DescriptorSet> descriptorSets;
        std::vector<vk::raii::DescriptorPool> descriptorPools;
        std::vector<std::shared_ptr<class ResourceReincarnation>> reincarnations;
        
        void clear();
    };
    TrashCan(class Swapchain&);
    ~TrashCan();
    void clearAll();
    void clear();
    
    uint32_t getFrameIndex()const;

    void trash(vk::raii::Buffer buffer,vk::raii::DeviceMemory deviceMemory);
    void trash(vk::raii::PipelineLayout pipelineLayout,vk::raii::Pipeline pipeline);
    void trash(vk::raii::DescriptorPool descriptorPool);
    void trash(vk::raii::DescriptorSet descriptorSet);
    void trash(std::shared_ptr<class ResourceReincarnation> reincarnation);
private:
    TrashLayer trashLayer[MAX_FRAMES_IN_FLIGHT];

};