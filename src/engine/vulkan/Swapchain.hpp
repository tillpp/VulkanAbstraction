#pragma once
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/Image.hpp"
#include "engine/vulkan/TrashCan.hpp"

typedef uint32_t ImageIndex;
class Swapchain{
    class Window* window = nullptr;
    Device* device = nullptr;
public:
    vk::SurfaceFormatKHR   surfaceFormat;
    vk::Extent2D           swapChainExtent;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<Image::Reincarnation> images;

    static void addRequirements(DeviceSetup& ds);
    void create(class Device& device,class Window&);
private: // helper: surface
    void createSwapchain();

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR   chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
    static vk::Extent2D         chooseSwapExtent(class Window& window,vk::SurfaceCapabilitiesKHR const &capabilities);
    static uint32_t             chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
public:
    TrashCan trashCan;

    Swapchain();
    void clear();
    ~Swapchain();
    
    /**
        TODO:
        screenSize dependend stuff should be recreated here. like Depthbuffer (but not all depthbuffer)
    */
    void recreate();
        
    [[nodiscard]]bool begin();
    void end();

        
    CommandBuffer& getCommandBuffer();
    uint32_t const& getFrameIndex()const;
    
    ImageIndex imageIndex;
private: // helper: rendering
    void createSync();
    uint32_t frameIndex = 0;
    
    std::vector<CommandBuffer>       commandBuffers;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     inFlightFences;

};