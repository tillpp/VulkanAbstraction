#pragma once
#include "common.hpp" // IWYU pragma: keep
#include <set>
#include <string>
#include <vector>

class InstanceSetup{
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif
    std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    friend class Instance;
public:
    std::set<std::string> layers;
    std::set<std::string> extensions;
    vk::InstanceCreateFlags flags{};

    InstanceSetup();

private: //helper
    void addValidationLayer();
    
};

//TODO: vulkan give objects name
class Instance{
    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;

    InstanceSetup setup;
public:

    void create(InstanceSetup& vulkanSetup, std::string ApplicationName);
    bool isCreated()const;

    operator vk::raii::Instance&();

    const InstanceSetup& getSetup();
private: // helper
    void printSystemInfo();
};