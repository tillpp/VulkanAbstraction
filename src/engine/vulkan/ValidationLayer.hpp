#pragma once
#include "engine/vulkan/Instance.hpp"

class ValidationLayer
{
    const std::vector<char const*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif
public:
    ValidationLayer(InstanceSettings* settings);
    ~ValidationLayer();
};