#include "Instance.hpp"
#include "vulkan/vulkan_core.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

Instance::operator vk::raii::Instance&(){
    return instance;
}

void Instance::create(InstanceSettings settings){
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName   = "Yet Another Voxel Game",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion         = vk::ApiVersion13,
    };

    {
        uint32_t apiVersion = VK_MAKE_VERSION(1, 0, 0);
        if(vkEnumerateInstanceVersion(&apiVersion) == VK_SUCCESS){
            uint32_t major = VK_VERSION_MAJOR(apiVersion);
            uint32_t minor = VK_VERSION_MINOR(apiVersion);

            std::cout << "Vulkan support: "<<major <<"."<< minor<<std::endl;
        }
    }
    // for macOS
    #ifdef __APPLE__
    {
        settings.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
        settings.extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    }
    #endif



    std::vector<const char*> extensions;
    std::vector<const char*> layers;

    for (auto& extension : settings.extensions) {
        extensions.push_back(extension.c_str());
    }
    for (auto& layer : settings.layers) {
        layers.push_back(layer.c_str());
    }

    vk::InstanceCreateInfo createInfo{
        .flags = settings.flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (uint32_t)layers.size(),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    {
        auto extensions = context.enumerateInstanceExtensionProperties();
        std::cout << "available extensions:\n";
        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }
    {
        auto extensions = context.enumerateInstanceLayerProperties();
        std::cout << "available Layer:\n";
        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.layerName << '\n';
        }
    }

    instance = vk::raii::Instance(context, createInfo);
}
bool Instance::isCreated()const{
    return instance != nullptr;
}
