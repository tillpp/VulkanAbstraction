#pragma once
#include <filesystem>
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/DepthBuffer.hpp"


class Pipeline
{
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(
        Device& device,
        const std::string& code) const;

    Swapchain* swapchain = nullptr;
public:
    vk::raii::PipelineLayout pipelineLayout   = nullptr;
    vk::raii::Pipeline       graphicsPipeline = nullptr;

    enum Stencil{
        writeStencil,
        readStencil,
        noStencil
    };
    void create(
        class Window& window,
        Device& device,
        std::filesystem::path shaderFile, 
        std::string entryFnVertex, 
        std::string entryFnFragment,
        vk::VertexInputBindingDescription bindingDescription,
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions,
        std::vector<class DescriptorSetLayout*> dsLayouts,
        DepthBuffer& depthBuffer, bool depthTesting,Stencil stencil,
        std::optional<class PushConstant*> pushConstant);
    ~Pipeline();

    void bind(CommandBuffer& cb,vk::PipelineBindPoint pipelineBindPoint);
};

