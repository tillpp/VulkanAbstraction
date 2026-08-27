#pragma once
#include "engine/vulkan/common.hpp"// IWYU pragma: keep
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/Pipeline.hpp"

class PushConstant{
    vk::ShaderStageFlagBits shaderStages;
    size_t offset;
public:
    vk::PushConstantRange pushConstantRange;

    void create(vk::ShaderStageFlagBits shaderStages,size_t offset,size_t size);

    template<class T>
    void use(CommandBuffer& cb,Pipeline& pipeline,const T& ptr){
        vkCmdPushConstants(*cb.commandBuffer,*pipeline.pipelineLayout,(VkShaderStageFlags)shaderStages,offset,sizeof(T),&ptr);  
    }
};