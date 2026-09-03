#include "Frame.hpp"
#include "vulkan/vulkan.hpp"

void Frame::create(Window& window,int texWidth,int texHeight){
    this->window = &window;
    this->texWidth = texWidth;
    this->texHeight = texHeight;

    depthBuffer.create(window, false, {this->texWidth,this->texHeight});
    image = std::make_shared<Image>();
    image->create(window,texWidth,texHeight, nullptr, true, 
        vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
        vk::ImageUsageFlags::BitsType::eColorAttachment | vk::ImageUsageFlagBits::eSampled
        // ,vk::SamplerCreateInfo{
        //    .magFilter = vk::Filter::eLinear, 
        //    .minFilter = vk::Filter::eLinear,  
        //   .mipmapMode = vk::SamplerMipmapMode::eLinear,
        //     .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        //     .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        //     .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        //    .mipLodBias = 6,
        //     .anisotropyEnable = vk::False, 
        //     .maxAnisotropy = 1.0f, 
        //     .compareEnable = vk::True,
        //     .compareOp = vk::CompareOp::eAlways,
        //    .minLod = 0,
        //    .maxLod = VK_LOD_CLAMP_NONE,
        // }
        ,vk::SamplerCreateInfo{
           .magFilter = vk::Filter::eNearest, 
           .minFilter = vk::Filter::eNearest,  
           .mipmapMode = vk::SamplerMipmapMode::eNearest,
           .addressModeU = vk::SamplerAddressMode::eClampToEdge,
           .addressModeV = vk::SamplerAddressMode::eClampToEdge,
           .addressModeW = vk::SamplerAddressMode::eClampToEdge,
           .anisotropyEnable = vk::True, 
           .maxAnisotropy = 16.0f, 
           .compareEnable = vk::True,
           .compareOp = vk::CompareOp::eAlways,
           .minLod = 0,
           .maxLod = 0,
        });
    CommandBuffer cb(window.commandPool);
    cb.beginSingleTimeCommands();
    for(auto& current:image->frames){
        current->transitionImageLayout(
            cb, 
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
            {},                                                     // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
            vk::ImageAspectFlagBits::eColor
        );
    }
    cb.endSingleTimeCommands(window.commandPool);
}
void Frame::begin(CommandBuffer& cb){
    auto current = std::dynamic_pointer_cast<Image::Reincarnation>(image->getResource(window->swapchain.getFrameIndex()));
    current->transitionImageLayout(
        cb, 
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                     // dstAccessMask
        vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
        vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        vk::ImageAspectFlagBits::eColor
    );
    current->beginRendering(cb, &depthBuffer);
    cb.commandBuffer.setViewport(0, vk::Viewport{
        .x = 0,
        .y = 0, 
        .width  = (float)texWidth,
        .height = (float)texHeight,
        .minDepth = 0,
        .maxDepth = 1,
    });
    cb.commandBuffer.setScissor(0, vk::Rect2D{
        .offset = {0,0},
        .extent = {texWidth,texHeight},
    });
}
void Frame::end(CommandBuffer& cb){
    auto current = std::dynamic_pointer_cast<Image::Reincarnation>(image->getResource(window->swapchain.getFrameIndex()));
    cb.commandBuffer.endRendering();
    current->transitionImageLayout(
        cb, 
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
        {},                                                     // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
        vk::ImageAspectFlagBits::eColor
    );
}
