#pragma once
#include <filesystem>
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
        class DescriptorSetLayout& dsLayout,
        Stencil stencil,
        DepthBuffer& depthBuffer, bool depthTesting = true,
        std::optional<class PushConstant*> pushConstant = {});
    ~Pipeline();
};

