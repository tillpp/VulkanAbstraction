#include "CommandBuffer.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/Queue.hpp"


CommandBuffer::CommandBuffer(CommandPool& pool){
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *pool.commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
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
    commandBuffer.begin({});
    
}
void CommandBuffer::end(){
    commandBuffer.end();
}
