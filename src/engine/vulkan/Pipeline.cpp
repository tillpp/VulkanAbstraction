#include "Pipeline.hpp"
#include "engine/vulkan/DescriptorLayout.hpp"
#include "engine/vulkan/Descriptor.hpp"
#include "engine/data/Filebasic.hpp"
#include "engine/vulkan/PushContant.hpp"
#include "engine/vulkan/Window.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <string>

[[nodiscard]] vk::raii::ShaderModule Pipeline::createShaderModule(
    Device& device,
    const std::string& code) const{

    vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(code[0]), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
    vk::raii::ShaderModule shaderModule{ device.device, createInfo };
    return shaderModule;
}
void Pipeline::create(
    class Window& window,
    Device& device,
    std::filesystem::path shaderFile, 
    std::string entryFnVertex, 
    std::string entryFnFragment, 
    vk::VertexInputBindingDescription bindingDescription,
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions,
    DescriptorSetLayout& dsLayout,
    Stencil stencil,
    DepthBuffer& depthBuffer,bool depthTesting,
    std::optional<PushConstant*> pushConstant
) {
    this->swapchain = &window.swapchain;
    // shader
    vk::raii::ShaderModule shaderModule = createShaderModule(device,readFileOrThrow(shaderFile));
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = entryFnVertex.c_str(),
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ 
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = entryFnFragment.c_str(),
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };
    
    //dynamic states
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    
    // basicly VAO        
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()};

    // assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };
    
    // viewport & scissors
    // vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f};
    // vk::Rect2D scissor{vk::Offset2D{ 0, 0 }, swapChainExtent};
    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    // alpha blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable         = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    if(stencil == writeStencil){
        colorBlendAttachment.colorWriteMask = {};
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment
    };
    
    // PipelineLayout (for uniforms later)
    {
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
            .setLayoutCount = 1, 
            .pSetLayouts = &*dsLayout.descriptorSetLayout, 
            .pushConstantRangeCount = 0 
        };
        if(pushConstant.has_value()){
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstant.value()->pushConstantRange;
        }
        pipelineLayout = vk::raii::PipelineLayout(device.device, pipelineLayoutInfo);
    }
    

    //enable depthtesting
    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False
    };
    if(stencil != noStencil){
        depthStencil.stencilTestEnable     = vk::True;
    }

    if(depthTesting){
        depthStencil.depthTestEnable       = vk::True;
        depthStencil.depthWriteEnable      = vk::True;
        depthStencil.depthCompareOp        = vk::CompareOp::eLessOrEqual;
    }else{
        depthStencil.depthTestEnable       = vk::False;
        depthStencil.depthWriteEnable      = vk::False;
        depthStencil.depthCompareOp        = vk::CompareOp::eAlways;
    }
    if(stencil == writeStencil)
    {
        vk::StencilOpState stencilWrite{
            .failOp    = vk::StencilOp::eKeep,
            .passOp    = vk::StencilOp::eReplace,
            .depthFailOp = vk::StencilOp::eKeep,
            .compareOp = vk::CompareOp::eAlways,
            .compareMask = 0xff,
            .writeMask   = 0xff,
            .reference   = 1
        };

        depthStencil.front = stencilWrite;
        depthStencil.back  = stencilWrite;
    }else if(stencil == readStencil){
        vk::StencilOpState stencilRead{
            .failOp      = vk::StencilOp::eKeep,
            .passOp      = vk::StencilOp::eKeep,
            .depthFailOp = vk::StencilOp::eKeep,
            .compareOp   = vk::CompareOp::eEqual,
            .compareMask = 0xff,
            .writeMask   = 0,
            .reference   = 1
        };
        
        depthStencil.front = stencilRead;
        depthStencil.back = stencilRead;
    }

    
    // dynamic rendering
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{ 
        .colorAttachmentCount = 1, 
        .pColorAttachmentFormats = &window.swapchain.surfaceFormat.format, 
        .depthAttachmentFormat = depthBuffer.depthFormat,
    };
    if(stencil != noStencil){
        assert(depthBuffer.hasStencil());
        pipelineRenderingCreateInfo.stencilAttachmentFormat = depthBuffer.depthFormat;
    }

    // everything together
    vk::GraphicsPipelineCreateInfo  graphicsPipelineCreateInfo{
        .stageCount          = 2,
        .pStages             = shaderStages,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = nullptr // we are using dynamic rendering instead
    };
    graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

    graphicsPipeline = vk::raii::Pipeline(device.device, nullptr, graphicsPipelineCreateInfo);
    // TODO: learn more about pipeline caching.
}
Pipeline::~Pipeline(){
    if(swapchain)
        swapchain->trashCan.trash(std::move(pipelineLayout),std::move(graphicsPipeline));
}
void Pipeline::bind(CommandBuffer& cb,vk::PipelineBindPoint pipelineBindPoint){
    cb.commandBuffer.bindPipeline(pipelineBindPoint, graphicsPipeline);
}
