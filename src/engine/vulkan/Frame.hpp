#pragma once
#include "common.hpp" // IWYU pragma: keep
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/DepthBuffer.hpp"
#include "engine/vulkan/Image.hpp"
#include "engine/vulkan/Window.hpp"
#include <cstdint>
#include <utility>
#include <vector>

class Frame{
    class Window* window;
    uint32_t texWidth, texHeight;
    std::vector<CommandBuffer> buffers;
public:
    std::shared_ptr<Image> image;
    DepthBuffer depthBuffer;

    void create(Window& window,int texWidth,int texHeight);

    /**
        CommandBuffer cb(window->commandPool);
        cb.beginSingleTimeCommands();
        frame.begin(cb);
        ... render ...
        frame.end(cb);
        cb.endSingleTimeCommands(window.commandPool);
    */
    CommandBuffer* begin();
    void end();

    virtual ~Frame();
};
