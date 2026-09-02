#pragma once
#include "engine/vulkan/CommandPool.hpp"


class CommandBuffer
{
public:
    vk::raii::CommandBuffer commandBuffer = nullptr;
    CommandBuffer(CommandPool& pool);

    void beginSingleTimeCommands();
    void endSingleTimeCommands(CommandPool& pool);

    void begin();
    void end();
    
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
};
