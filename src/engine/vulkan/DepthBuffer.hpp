#pragma once
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/Image.hpp"

class DepthBuffer
{
    static vk::Format findSupportedFormat(Device& device,const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features,bool withStencil);
    static bool hasStencilComponent(vk::Format format);
    static vk::Format findDepthFormat(Device& device,bool withStencil);

    Swapchain* swapchain = nullptr;
    bool withStencil = false;
public:
    vk::Format depthFormat;
    
    Image image;

    void create(class Window& window,bool withStencil,vk::Extent2D extent);
    void recreate(class Window& window,vk::Extent2D extent);
    ~DepthBuffer();

    bool hasStencil()const;

};
