#include "Image.hpp"
#include "engine/vulkan/CommandPool.hpp"
#include "engine/vulkan/DepthBuffer.hpp"
#include "engine/vulkan/Window.hpp"
#include "engine/vulkan/Buffer.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <iostream>
#include <memory>

void Image::Reincarnation::initExisitingImage(const vk::Image& image,vk::Extent2D extent){
    this->image = image;
    this->extent = extent;
}

void Image::Reincarnation::initImage(Device& device,uint32_t width, uint32_t height, 
    vk::Format format, vk::SampleCountFlagBits samples, vk::ImageTiling tiling, vk::ImageUsageFlags usage, 
    vk::MemoryPropertyFlags properties) {
    this->extent = vk::Extent2D{.width = width, .height = height};


    //
    vk::ImageCreateInfo imageInfo{ 
        .imageType = vk::ImageType::e2D, 
        .format = format,
        .extent = {width, height, 1}, 
        .mipLevels = 1, .arrayLayers = 1,
        .samples = samples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive 
    };
        
    imageOwner = vk::raii::Image(device.device, imageInfo);
    
    vk::MemoryRequirements memRequirements = imageOwner.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{ 
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = Buffer::findMemoryType(device,memRequirements.memoryTypeBits, properties) 
    };
    imageMemory = vk::raii::DeviceMemory(device.device, allocInfo);
    imageOwner.bindMemory(*imageMemory, 0);       
    image = imageOwner;
}
void Image::Reincarnation::transitionImageLayout(
        CommandBuffer& commandBuffer,
        vk::ImageLayout oldLayout, 
        vk::ImageLayout newLayout,
        vk::AccessFlags2        src_access_mask,
	    vk::AccessFlags2        dst_access_mask,
	    vk::PipelineStageFlags2 src_stage_mask,
	    vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags    image_aspect_flags) {

    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = {
            .aspectMask     = image_aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1}};
    vk::DependencyInfo dependency_info = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier};
    commandBuffer.commandBuffer.pipelineBarrier2(dependency_info);
}
void Image::Reincarnation::copyBufferToImage(CommandBuffer& commandBuffer,const vk::raii::Buffer& buffer, uint32_t width, uint32_t height) {
    
    vk::BufferImageCopy region{ 
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, 
        .imageOffset = {0, 0, 0}, 
        .imageExtent = {width, height, 1} 
    };
    commandBuffer.commandBuffer.copyBufferToImage(buffer,image,vk::ImageLayout::eTransferDstOptimal,{region});
}

void Image::Reincarnation::createImageView(Device& device, vk::Format format, vk::ImageAspectFlags aspectFlags) {
    vk::ImageViewCreateInfo viewInfo{ 
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format, 
        .subresourceRange = { aspectFlags, 0, 1, 0, 1 } 
    };
    imageView = vk::raii::ImageView( device.device, viewInfo );
}
void Image::Reincarnation::createTextureSampler(Device& device, vk::SamplerCreateInfo samplerInfo) {
    vk::PhysicalDeviceProperties properties = device.physicalDevice.getProperties();
    //  {
    //     .magFilter = vk::Filter::eLinear, 
    //     .minFilter = vk::Filter::eLinear,  
    //     .mipmapMode = vk::SamplerMipmapMode::eLinear,
    //     .addressModeU = vk::SamplerAddressMode::eRepeat,
    //     .addressModeV = vk::SamplerAddressMode::eRepeat,
    //     .addressModeW = vk::SamplerAddressMode::eRepeat,
    //     .anisotropyEnable = vk::True, // TODO: deactive when .samplerAnisotropy isnt available and make maxAnisotropy = 1.0f
    //     .maxAnisotropy = properties.limits.maxSamplerAnisotropy, 
    //     .compareEnable = vk::False,
    //     .compareOp = vk::CompareOp::eAlways
    // };

    textureSampler = vk::raii::Sampler(device.device, samplerInfo);
}
void Image::Reincarnation::beginRendering(CommandBuffer& commandBuffer,DepthBuffer* depthBuffer){
    // Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
    transitionImageLayout(
        commandBuffer, 
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                        // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // dstStage
        vk::ImageAspectFlagBits::eColor
    );

    if(depthBuffer){
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth;
        if(depthBuffer->hasStencil()){
            aspect |= vk::ImageAspectFlagBits::eStencil;
        }
        (*depthBuffer).image.current->transitionImageLayout(
            commandBuffer,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            aspect);
    }

    vk::ClearValue clearColor = vk::ClearColorValue(0.01f, 0.01f, 0.01f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue{
        .depth = 1.0f,
        .stencil = 0
    };
    

    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView   = imageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = extent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo,
    };
    vk::RenderingAttachmentInfo depthAttachmentInfo;
    if(depthBuffer){
        depthAttachmentInfo = {
            .imageView   = (*depthBuffer).image.current->imageView,
            .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp      = vk::AttachmentLoadOp::eClear,
            .storeOp     = vk::AttachmentStoreOp::eDontCare,
            .clearValue  = clearDepth
        };
        renderingInfo.pDepthAttachment     = &depthAttachmentInfo;
        if(depthBuffer->hasStencil())
            renderingInfo.pStencilAttachment   = &depthAttachmentInfo;
    }
    commandBuffer.commandBuffer.beginRendering(renderingInfo);
}
void Image::Reincarnation::endRendering(CommandBuffer& commandBuffer){
    commandBuffer.commandBuffer.endRendering();

    // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
    transitionImageLayout(
        commandBuffer, 
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
        {},                                                     // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
        vk::ImageAspectFlagBits::eColor
    );
}

DescriptorInfo Image::Reincarnation::getDescriptorInfo()const{
    DescriptorInfo di;
    di.type = DescriptorInfo::IMAGE;
    di.imageInfo = { 
        .sampler     = textureSampler, 
        .imageView   = imageView, 
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    return di;
}
std::shared_ptr<Image::Reincarnation> Image::getCurrent(){
    return current;
}

void Image::create(Window& window,std::filesystem::path path, vk::SamplerCreateInfo samplerInfo){        
    //load image
    int texWidth = 0, texHeight = 0, texChannels = 0;
    auto pathAsString = path.string();
    stbi_uc* pixels = stbi_load(pathAsString.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    create(window,texWidth, texHeight,pixels,false,vk::Format::eR8G8B8A8Srgb,vk::SampleCountFlagBits::e1,vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, samplerInfo);
    stbi_image_free(pixels);
}
void Image::create(Window& window,int texWidth, int texHeight,std::optional<stbi_uc*> pixels,bool foreachFrame, vk::Format format, vk::SampleCountFlagBits samples,vk::ImageUsageFlags usage,vk::SamplerCreateInfo samplerInfo){

    auto& device = window.commandPool.getDevice();
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    // load into stating Buffer
    Buffer stagingBuffer;
    if(pixels.has_value())
    {
        assert(pixels.has_value());
        stagingBuffer.create(
            window,
            device,
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        
        void* data = stagingBuffer.bufferMemory.mapMemory(
            0, imageSize);
        memcpy(data, pixels.value(), imageSize);
        stagingBuffer.bufferMemory.unmapMemory();
    }
    
    auto fun = [&](std::shared_ptr<Reincarnation> current){
        current->initImage(device, texWidth, texHeight, 
        format,samples, vk::ImageTiling::eOptimal,
         usage, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
        CommandBuffer commandBuffer(window.commandPool);

        if(pixels.has_value()){
            commandBuffer.beginSingleTimeCommands();
            current->transitionImageLayout(
                commandBuffer, 
                vk::ImageLayout::eUndefined, 
                vk::ImageLayout::eTransferDstOptimal,
                {},
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::ImageAspectFlagBits::eColor
            );
            current->copyBufferToImage(commandBuffer,stagingBuffer.buffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
            current->transitionImageLayout(
                commandBuffer, 
                vk::ImageLayout::eTransferDstOptimal, 
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::AccessFlagBits2::eTransferWrite,
                vk::AccessFlagBits2::eShaderRead,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::ImageAspectFlagBits::eColor
            );   
            commandBuffer.endSingleTimeCommands(window.commandPool);
        }

        current->createImageView(
            device, 
            format,
            vk::ImageAspectFlagBits::eColor);

        current->createTextureSampler(device, samplerInfo);
    };

    
    if(foreachFrame){
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto v = std::make_shared<Reincarnation>();
            fun(v);
            frames.push_back(v);
        }
    }else{
        current = std::make_shared<Reincarnation>();
        fun(current);
    }
    notifyDescriptorSet();
}
std::shared_ptr<ResourceReincarnation> Image::getResource(size_t frameIndex)const{
    if(frames.size()){
        return frames[frameIndex];
    }
    return current;
}