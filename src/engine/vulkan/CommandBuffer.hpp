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
    
};
