#pragma once
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/Image.hpp"

class Swapchain
{
public:
    vk::SurfaceFormatKHR   surfaceFormat;
    vk::Extent2D           swapChainExtent;

    vk::raii::SwapchainKHR swapChain = nullptr;
    
    std::vector<Image::Reincarnation> images;

    Swapchain(DeviceSettings& deviceSettings);
    ~Swapchain();

    void create(class Window& window,Device& device);
    void recreate(class Window& window,Device& device);

private:
    // choose surface settings
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static  vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
    static  vk::Extent2D chooseSwapExtent(class Window& window,vk::SurfaceCapabilitiesKHR const &capabilities);
    static  uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
        
};
