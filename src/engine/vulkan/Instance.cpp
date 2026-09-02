#include "Instance.hpp"
#include <cassert>
#include <set>
#include <string>
#include <vector>


InstanceSetup::InstanceSetup(){
    if (enableValidationLayers)
        addValidationLayer();
    
    // for macOS
    #ifdef __APPLE__
    {
        settings.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
        settings.extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    }
    #endif
}
void InstanceSetup::addValidationLayer(){
    // check support
    vk::raii::Context  context;
    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(validationLayers,
        [&layerProperties](auto const &requiredLayer) {
        return std::ranges::none_of(layerProperties,
        [requiredLayer](auto const &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
    });
    if (unsupportedLayerIt != validationLayers.end())
    {
        std::cerr << ("Required layer not supported: " + std::string(*unsupportedLayerIt)) << std::endl;
        return;
        //throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }
    std::cout << "ValidationLayer available and active!" << std::endl;
    layers.insert(validationLayers.begin(), validationLayers.end());
}


void Instance::create(InstanceSetup& vulkanSetup, std::string ApplicationName){
    this->setup = vulkanSetup;
    vk::ApplicationInfo appInfo{
        .pApplicationName   = ApplicationName.c_str(),
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion         = vk::ApiVersion13,
    };
    printSystemInfo();
    
    std::vector<const char*> extensions;
    std::vector<const char*> layers;

    for (auto& extension : vulkanSetup.extensions) {
        extensions.push_back(extension.c_str());
    }
    for (auto& layer : vulkanSetup.layers) {
        layers.push_back(layer.c_str());
    }

    vk::InstanceCreateInfo createInfo{
        .flags                   = vulkanSetup.flags,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = (uint32_t)layers.size(),
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = (uint32_t)extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };
    instance = vk::raii::Instance(context, createInfo);
}
bool Instance::isCreated()const{
    return instance != nullptr;
}
Instance::operator vk::raii::Instance&(){
    return instance;
}
void Instance::printSystemInfo(){
    uint32_t apiVersion = VK_MAKE_VERSION(1, 0, 0);
    if(vkEnumerateInstanceVersion(&apiVersion) == VK_SUCCESS){
        uint32_t major = VK_VERSION_MAJOR(apiVersion);
        uint32_t minor = VK_VERSION_MINOR(apiVersion);

        std::cout << "Vulkan support: "<<major <<"."<< minor<<std::endl;
    }
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
}
const InstanceSetup& Instance::getSetup(){
    return setup;
}
