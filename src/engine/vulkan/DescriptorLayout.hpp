#pragma once
#include "engine/vulkan/Device.hpp"


struct DescriptorLayout{
    uint32_t             binding;
    vk::ShaderStageFlags stageFlags;
    vk::DescriptorType   descriptorType;
    uint32_t             descriptorCount;
    bool                 variableCount;

    DescriptorLayout(
        uint32_t             binding,
        vk::ShaderStageFlags stageFlags,
        vk::DescriptorType   descriptorType,
        uint32_t             descriptorCount,
        bool                 variableCount = false
    );
    
    vk::DescriptorSetLayoutBinding getBinding()const;
};

class DescriptorSetLayout
{
public:
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    void create(Device& device,std::vector<DescriptorLayout> dsArray);
};
