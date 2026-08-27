#pragma once
#include "engine/vulkan/RenderSync.hpp"
#include "engine/vulkan/Image.hpp"


//TODO: look into stencil tests
class DepthBuffer
{
    static vk::Format findSupportedFormat(Device& device,const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features,bool withStencil);
    static bool hasStencilComponent(vk::Format format);
    static vk::Format findDepthFormat(Device& device,bool withStencil);

    RenderSync* render = nullptr;
    bool withStencil = false;
public:
    
    Image image;
    vk::Format depthFormat;

    void create(class Window& window,bool withStencil);
    void recreate(class Window& window);
    ~DepthBuffer();

    bool hasStencil()const;

};
