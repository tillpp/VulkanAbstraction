#pragma once
#include "engine/vulkan/common.hpp" // IWYU pragma: keep

#include <optional>
#include <stb_image.h>
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/CommandBuffer.hpp"
#include <filesystem>
#include "engine/vulkan/Descriptor.hpp"
#include "vulkan/vulkan.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.hpp>

//TODO learn more about ktx
//TODO: try to reread https://docs.vulkan.org/tutorial/latest/06_Texture_mapping/00_Images.html#_layout_transitions , cause i didnt understand everything.

class Image:public Resource{
public:
    // when the image gets resized, it "reincarnates" (wink). 
    // DS can still point at old Reincarnations before their are updated.
    struct Reincarnation:public ResourceReincarnation{    
        vk::Image              image          = nullptr;
        vk::raii::Image        imageOwner     = nullptr;
        vk::raii::DeviceMemory imageMemory    = nullptr;
        
        vk::Extent2D           extent;

        vk::raii::ImageView    imageView      = nullptr;
        vk::raii::Sampler      textureSampler = nullptr;

        void initExisitingImage(const vk::Image& image,vk::Extent2D extent);
        void initImage(Device& device,uint32_t width, uint32_t height, 
            vk::Format format,vk::SampleCountFlagBits samples, vk::ImageTiling tiling, vk::ImageUsageFlags usage, 
            vk::MemoryPropertyFlags properties);
        
        void createImageView(Device& device, vk::Format format, vk::ImageAspectFlags aspectFlags) ;
        void createTextureSampler(Device& device, vk::SamplerCreateInfo samplerInfo);
            
        //TODO: store the current layout to be used for "oldLayout" (Maybe that's not a good idea?)
        void transitionImageLayout(
            CommandBuffer& cb,
            vk::ImageLayout oldLayout, 
            vk::ImageLayout newLayout,
            vk::AccessFlags2        src_access_mask,
	        vk::AccessFlags2        dst_access_mask,
	        vk::PipelineStageFlags2 src_stage_mask,
	        vk::PipelineStageFlags2 dst_stage_mask,
            vk::ImageAspectFlags    image_aspect_flags
        );
        void copyBufferToImage(CommandBuffer& cb,const vk::raii::Buffer& buffer, uint32_t width, uint32_t height);
        
        void beginRendering(CommandBuffer& commandBuffer,class DepthBuffer* depthBuffer);
        void endRendering(CommandBuffer& commandBuffer);

        virtual DescriptorInfo getDescriptorInfo()const;
    };
    std::shared_ptr<Reincarnation> current;
    std::vector<std::shared_ptr<Reincarnation>> frames;
public:
    std::shared_ptr<Reincarnation> getCurrent();

    Image()=default;
    virtual ~Image()=default;

    void create(class Window& window,std::filesystem::path path, vk::SamplerCreateInfo samplerInfo = {
        .magFilter = vk::Filter::eNearest, 
        .minFilter = vk::Filter::eNearest,  
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .anisotropyEnable = vk::False, 
        .maxAnisotropy = 1.0f, 
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0,
        .maxLod = 0,
    });
    void create(class Window& window,int texWidth, int texHeight,std::optional<stbi_uc*> pixels,bool foreachFrame, vk::Format, vk::SampleCountFlagBits samples,vk::ImageUsageFlags usage,vk::SamplerCreateInfo samplerInfo = {
        .magFilter = vk::Filter::eNearest, 
        .minFilter = vk::Filter::eNearest,  
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .anisotropyEnable = vk::False, 
        .maxAnisotropy = 1.0f, 
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0,
        .maxLod = 0,
    });
    
    virtual std::shared_ptr<ResourceReincarnation> getResource(size_t frameIndex)const override;
private:
};