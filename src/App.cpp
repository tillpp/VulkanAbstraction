#include "App.hpp"
#include "GLFW/glfw3.h"
#include "engine/vulkan/Buffer.hpp"
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/DepthBuffer.hpp"
#include "engine/vulkan/Descriptor.hpp"
#include "engine/vulkan/Image.hpp"
#include "engine/vulkan/PushContant.hpp"
#include "engine/vulkan/ValidationLayer.hpp"
#include "engine/vulkan/DefaultVertex.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <linux/limits.h>
#include <memory>

App* App::app = nullptr;

App::App(std::filesystem::path projectDir):projectDir(projectDir),
window(&settings,&dSettings){
    assert(!app);
    app = this;

    initVulkan();
}
void App::initVulkan(){
    ValidationLayer validationLayer(&settings);
    instance.create( settings);
    window.create(instance, &dSettings, 1280, 720, "yo");

    // Create a chain of feature structures
    vk::PhysicalDeviceFeatures2 a{.features = {.samplerAnisotropy = true,}};// vk::PhysicalDeviceFeatures2 (empty for now)
    vk::PhysicalDeviceVulkan13Features b{.synchronization2 = true,.dynamicRendering = true}; // Enable dynamic rendering from Vulkan 1.3
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT c{.extendedDynamicState = true };  // Enable extended dynamic state from the extension
    vk::PhysicalDeviceVulkan11Features d{.shaderDrawParameters = true};
    a.pNext = &b;
    b.pNext = &c;
    c.pNext = &d;
    
    DeviceFeatures dfeatures(&a,[&](const vk::raii::PhysicalDevice & physicalDevice)->bool
    {
        // Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
        auto features = physicalDevice .template getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
            vk::PhysicalDeviceVulkan11Features
        >();
        bool supportsRequiredFeatures = 
            features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState &&
            features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters;
        return supportsRequiredFeatures;
    });
    

    device.create(instance, dSettings,dfeatures);
}
bool App::run(){
    while(auto cb = window.update()){    
        
        cb->commandBuffer.setViewport(0, vk::Viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>( window.swapChain.swapChainExtent.width),
            .height = static_cast<float>(window.swapChain.swapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        });
        cb->commandBuffer.setScissor(0, vk::Rect2D{
            .offset = vk::Offset2D{.x = 0,.y = 0},
            .extent = window.swapChain.swapChainExtent,
        });

    
        glfwPollEvents();

    }
    window.inputHandler.reset();

    return false;
}