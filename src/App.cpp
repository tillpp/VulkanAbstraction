#include "App.hpp"
#include "GLFW/glfw3.h"
#include "engine/vulkan/Buffer.hpp"
#include "engine/vulkan/DefaultVertex.hpp"
#include "engine/vulkan/Descriptor.hpp"
#include "engine/vulkan/DescriptorLayout.hpp"
#include "engine/vulkan/Frame.hpp"
#include "engine/vulkan/GraphicsQueue.hpp"
#include "engine/vulkan/Image.hpp"
#include "engine/vulkan/Instance.hpp"
#include "engine/vulkan/Pipeline.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <cmath>
#include <linux/limits.h>
#include <memory>

App* App::app = nullptr;

App::App(std::filesystem::path projectDir):projectDir(projectDir){
    assert(!app);
    app = this;

    initVulkan();
}
void App::initVulkan(){
    InstanceSetup vs;
    Window::addRequirements(vs);
    instance.create(vs, "Just a Game");
    DeviceSetup ds;
    window.create(instance, ds,1280, 720, "hello there");

    struct:public PhyDeviceFeatures{
        virtual bool physicalDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)override{
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
        }
    }df;
    // Create a chain of logical feature structures
    vk::PhysicalDeviceFeatures2 a{.features = {.samplerAnisotropy = true,}};// vk::PhysicalDeviceFeatures2 (empty for now)
    vk::PhysicalDeviceVulkan13Features b{.synchronization2 = true,.dynamicRendering = true}; // Enable dynamic rendering from Vulkan 1.3
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT c{.extendedDynamicState = true };  // Enable extended dynamic state from the extension
    vk::PhysicalDeviceVulkan11Features d{.shaderDrawParameters = true};
    a.pNext = &b;
    b.pNext = &c;
    c.pNext = &d;

    
    device.create(instance,df,&a,ds);

}
bool App::run(){
    Buffer bda;
    {
        DefaultVertex data[3];
        bda.createAndUpload(window, data, 3*sizeof(DefaultVertex), vk::BufferUsageFlags::BitsType::eStorageBuffer | vk::BufferUsageFlags::BitsType::eShaderDeviceAddressKHR);
    }

    Pipeline pipeline;
    DescriptorSetLayout dsl;
    Buffer buffer;
    std::shared_ptr<Image> image = std::make_shared<Image>();

    image->create(window, "assets/deleteme.png");    

    DescriptorSet ds,ds2;
    
    DefaultVertex data[3];
    buffer.createAndUpload(window, data, 3*sizeof(DefaultVertex), vk::BufferUsageFlags::BitsType::eVertexBuffer);

    dsl.create(device, {
        DescriptorLayout(0,vk::ShaderStageFlagBits::eFragment,vk::DescriptorType::eCombinedImageSampler)
    });
    
    pipeline.create(
        window, device, 
        projectDir/"bin/shaders/shader.spv", 
        "vertMain", "fragMain", 
        DefaultVertex::getBindingDescription(), DefaultVertex::getAttributeDescriptions(),
        dsl, Pipeline::noStencil, window.depthBuffer, false, {});

    
    ds.create(device, window, dsl, {
        DescriptorLayout(0,vk::ShaderStageFlagBits::eFragment,vk::DescriptorType::eCombinedImageSampler)
    });
    ds.setResource(0, image);
    
    Frame frame;
    frame.create(window, 720, 1280);
    ds2.create(device, window, dsl, {
        DescriptorLayout(0,vk::ShaderStageFlagBits::eFragment,vk::DescriptorType::eCombinedImageSampler)
    });
    ds2.setResource(0, frame.image);

    while(auto cb = window.update()){
        {
            CommandBuffer cb(window.commandPool);
            cb.beginSingleTimeCommands();
            frame.begin(cb);
            
            pipeline.bind(cb,vk::PipelineBindPoint::eGraphics);
            buffer.bindAsVertexBuffers(cb, 0);
            ds.bind(device, cb.commandBuffer, window, pipeline);
            cb.draw(3, 1, 0, 0);

            frame.end(cb);
            cb.endSingleTimeCommands(window.commandPool);
        }

        cb->commandBuffer.setViewport(0, vk::Viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>( window.swapchain.swapChainExtent.width),
            .height = static_cast<float>(window.swapchain.swapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        });
        cb->commandBuffer.setScissor(0, vk::Rect2D{
            .offset = vk::Offset2D{.x = 0,.y = 0},
            .extent = window.swapchain.swapChainExtent,
        });

        pipeline.bind(*cb,vk::PipelineBindPoint::eGraphics);
        buffer.bindAsVertexBuffers(*cb, 0);
        ds2.bind(device, cb->commandBuffer, window, pipeline);
        cb->draw(3, 1, 0, 0);

        glfwPollEvents();
    }
    window.inputHandler.reset();
    return false;
}