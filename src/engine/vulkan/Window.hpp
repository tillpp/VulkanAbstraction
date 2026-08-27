#pragma once
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/DepthBuffer.hpp"
#include "engine/vulkan/GraphicsQueue.hpp"
#include "engine/vulkan/RenderSync.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/common.hpp"  // IWYU pragma: keep
#include "engine/vulkan/Instance.hpp"
#include "engine/vulkan/Event.hpp"
#include <memory>


class Window
{
    GLFWwindow *window = nullptr;
    
    struct{
        int xpos,ypos;
        int sizex,sizey;
    }beforeFullscreen;
    
    bool grabMouse = true;
public:
    vk::raii::SurfaceKHR surface = nullptr;
    
    Window(InstanceSettings* settings,DeviceSettings* dSettings);
    Window(const Window&)=delete;
    ~Window();

    void create(Instance& instance,DeviceSettings* deviceSettings, int width, int height, const char *title);
    void close();
    CommandBuffer* update();
    
    operator GLFWwindow*();
    
    void toggleFullscreen();
    void toggleMouseGrab();

    bool shouldRecreateSwapchain = false;
    bool isMouseGrabbed()const;

    std::shared_ptr<InputHandler> inputHandler;

    Swapchain      swapChain;
    GraphicsQueue  gQueue;
    CommandPool    commandPool;
    RenderSync     render;
    CommandBuffer* currentCB = nullptr;
    DepthBuffer depthBuffer;
    

};
            