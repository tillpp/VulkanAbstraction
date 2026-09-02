#pragma once
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/GraphicsQueue.hpp"
#include "engine/vulkan/Instance.hpp"
#include "engine/vulkan/Window.hpp"
#include <filesystem>

class App{
    std::filesystem::path projectDir;
public:
    App(std::filesystem::path projectDir);
    
    Instance      instance;
    Device        device;
    Window        window;

    void initVulkan();
    
    ///@return restart?
    [[nodiscard]] bool run();     
    static App* app;
};