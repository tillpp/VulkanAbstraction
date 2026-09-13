#include "DepthBuffer.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/Image.hpp"
#include "engine/vulkan/Window.hpp"
#include <memory>

vk::Format DepthBuffer::findSupportedFormat(Device& device,const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, bool withStencil) {
    for (const auto format : candidates) {
        vk::FormatProperties props = device.physicalDevice.getFormatProperties(format);

        if(withStencil && !hasStencilComponent(format))
            continue;

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}
bool DepthBuffer::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}
vk::Format DepthBuffer::findDepthFormat(Device& device,bool withStencil) {
    return findSupportedFormat(
            device,
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment,
            withStencil
        );
}
void DepthBuffer::create(class Window& window,bool withStencil,vk::Extent2D extent){
    this->withStencil = withStencil;
    this->swapchain = &window.swapchain;
    recreate(window,extent);    
}
void DepthBuffer::recreate(class Window& window,vk::Extent2D extent){
    depthFormat = findDepthFormat(window.commandPool.getDevice(),withStencil);
    if(image.current)
        swapchain->trashCan.trash(image.getCurrent());
    image.current = std::make_shared<Image::Reincarnation>();
    image.current->initImage(window.commandPool.getDevice(),
        extent.width, 
        extent.height, 
        depthFormat, 
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment, 
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    vk::ImageAspectFlags imageAspect = vk::ImageAspectFlagBits::eDepth;
    if(withStencil) 
        imageAspect |= vk::ImageAspectFlagBits::eStencil; 

    image.current->createImageView(window.commandPool.getDevice(), 
        depthFormat, 
        imageAspect
    );
}
DepthBuffer::~DepthBuffer(){
    if(swapchain){
        swapchain->trashCan.trash(image.getCurrent());
    }   
}
bool DepthBuffer::hasStencil()const{
    return withStencil;
}
