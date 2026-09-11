#include "engine/vulkan/DescriptorLayout.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

vk::DescriptorSetLayoutBinding DescriptorLayout::getBinding()const{
    return vk::DescriptorSetLayoutBinding{
        .binding            = binding,
        .descriptorType     = descriptorType,
        .descriptorCount    = descriptorCount,
        .stageFlags         = stageFlags,
        .pImmutableSamplers = nullptr,
    };
}
DescriptorLayout::DescriptorLayout(
    uint32_t             binding,
    vk::ShaderStageFlags stageFlags,
    vk::DescriptorType   descriptorType,
    uint32_t             descriptorCount,
    bool                 variableCount
){
    this->binding         = binding;
    this->stageFlags      = stageFlags;
    this->descriptorType  = descriptorType;
    this->descriptorCount = descriptorCount;
    this->variableCount   = variableCount;
}
void DescriptorSetLayout::create(Device& device,std::vector<DescriptorLayout> dsArray){
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        uint32_t highestBinding = dsArray.size()? dsArray[0].binding:0;
        for(auto& ds:dsArray){
            bindings.push_back(ds.getBinding());

            if(ds.binding > highestBinding)
                highestBinding = ds.binding;
        }
        for(auto& ds:dsArray){
            if(ds.binding != highestBinding && ds.variableCount)
                assert(0 && "only last descriptor bind can be variable count");
        }

        //activate DescriptorIndexing
        vk::DescriptorSetLayoutBindingFlagsCreateInfo flags;
        std::vector<vk::DescriptorBindingFlags> bindingFlags;
        {
            for (auto& ds : dsArray) {   
                if(ds.variableCount)
                    bindingFlags.push_back(
                        vk::DescriptorBindingFlags::BitsType::ePartiallyBound |
                        vk::DescriptorBindingFlags::BitsType::eVariableDescriptorCount);
                else 
                    bindingFlags.push_back({});
            }
            flags.setBindingFlags(bindingFlags);
        }
               
        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .pNext        = flags,
            .bindingCount = (uint32_t)bindings.size(), 
            .pBindings    = bindings.data(),
        };
        descriptorSetLayout = vk::raii::DescriptorSetLayout(device.device, layoutInfo);
    }        
}
