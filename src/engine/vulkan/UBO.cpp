#include "engine/vulkan/UBO.hpp"
#include "engine/vulkan/Window.hpp"

DescriptorInfo UBO::Reincarnation::getDescriptorInfo()const{
    DescriptorInfo di;
    di.type = DescriptorInfo::BUFFER;
    di.bufferInfo = { 
        .buffer = buffer.buffer, 
        .offset = 0, 
        .range  = size, 
    };
    return di;     
}   
void UBO::create(Window& window,Device& device,vk::DeviceSize bufferSize){
    // for memory
    frames.clear();

    for (size_t i = 0; i < window.render.MAX_FRAMES_IN_FLIGHT; i++) {
        
        auto reincarnation = std::make_shared<Reincarnation>();
        frames.push_back(reincarnation);
        reincarnation->buffer.create(window,device,bufferSize,vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        reincarnation->buffersMapped = std::move(reincarnation->buffer.bufferMemory.mapMemory(0, bufferSize));
        reincarnation->size          = bufferSize;
    }
}
std::shared_ptr<ResourceReincarnation> UBO::getResource(size_t frameIndex)const{
    return frames[frameIndex];
}