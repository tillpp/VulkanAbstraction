#pragma once
#include "engine/vulkan/CommandPool.hpp"
#include "engine/vulkan/DepthBuffer.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/Event.hpp"
#include "engine/vulkan/GraphicsQueue.hpp"
#include "engine/vulkan/Instance.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/common.hpp"  // IWYU pragma: keep

class Window:public DeviceCallback{
    GLFWwindow *window = nullptr;
    bool grabMouse = true;
    
    struct{
        int xpos,ypos;
        int sizex,sizey;
    }beforeFullscreen;
public:
    vk::raii::SurfaceKHR surface = nullptr;
    static void addRequirements(InstanceSetup&);

    Window();
    Window(const Window&)=delete;
    ~Window();

    bool shouldRecreateSwapchain = false;

    void create(Instance&,DeviceSetup&,int width, int height, const char *title);
    virtual void afterDeviceInit(class Device&)override;
    void close();


    GraphicsQueue gQueue;
    CommandPool   commandPool;
    Swapchain     swapchain;
    DepthBuffer   depthBuffer;

    CommandBuffer* update();
    CommandBuffer* currentCB = nullptr;
    void beginRendering(CommandBuffer*);
    void   endRendering(CommandBuffer*);


    std::shared_ptr<InputHandler> inputHandler;

    operator GLFWwindow*();

    void toggleFullscreen();
    void toggleMouseGrab();

    bool isMouseGrabbed()const;

private: //helper
    void initEventCallback();
};