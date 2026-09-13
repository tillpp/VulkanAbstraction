#include "CommandBuffer.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/Queue.hpp"
#include "vulkan/vulkan.hpp"


CommandBuffer::CommandBuffer(CommandPool& pool,vk::CommandBufferLevel level){
    vk::CommandBufferAllocateInfo allocInfo{ 
        .commandPool = *pool.commandPool, 
        .level = level, 
        .commandBufferCount = 1 
    };
    commandBuffer = std::move(vk::raii::CommandBuffers(pool.getDevice().device, allocInfo).front());
}
void CommandBuffer::beginSingleTimeCommands(){
    vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);
}
void CommandBuffer::endSingleTimeCommands(CommandPool& pool) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
    pool.getQueue().submit(submitInfo, nullptr);
    pool.getQueue().waitIdle(); //TODO: use fence  instead to copy multiple buffers at once.
}

void CommandBuffer::begin(){
    vk::CommandBufferInheritanceRenderingInfoKHR iri{};
    vk::CommandBufferInheritanceInfo i;
    i.pNext = iri;
    commandBuffer.begin({
        .pInheritanceInfo = &i,
    });
    
}
void CommandBuffer::end(){
    commandBuffer.end();
}

void CommandBuffer::execute(CommandBuffer& cb){
    commandBuffer.executeCommands({cb.commandBuffer});
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance){
    commandBuffer.draw(vertexCount,instanceCount,firstVertex,firstInstance);
}
