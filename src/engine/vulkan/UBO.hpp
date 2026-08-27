#pragma once
#include "engine/vulkan/common.hpp"// IWYU pragma: keep
#include "engine/vulkan/Descriptor.hpp"
#include "engine/vulkan/Device.hpp"
#include "engine/vulkan/RenderSync.hpp"
#include "engine/vulkan/Buffer.hpp"
#include <cstddef>
#include <memory>

class UBO:public Resource
{
    struct Reincarnation:public ResourceReincarnation{
        Buffer buffer;
        void* buffersMapped;
        size_t size;

        virtual DescriptorInfo getDescriptorInfo()const;
    };
public:
    std::vector<std::shared_ptr<Reincarnation>> frames;

    void create(class Window& window,Device& device,vk::DeviceSize bufferSize);
    virtual std::shared_ptr<ResourceReincarnation> getResource(size_t frameIndex)const override;

};

