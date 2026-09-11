#pragma once
#include "engine/vulkan/common.hpp"// IWYU pragma: keep
#include "engine/vulkan/DescriptorLayout.hpp"
#include <cstddef>
#include <map>
#include <memory>
#include <vector>


class DescriptorInfo{
public:    
    enum InfoType{
        BUFFER,
        IMAGE,
    };
    InfoType type;

    vk::DescriptorBufferInfo bufferInfo;
    vk::DescriptorImageInfo imageInfo;
};

struct ResourceReincarnation{
    virtual DescriptorInfo getDescriptorInfo()const = 0;
};
struct Resource{
    virtual std::shared_ptr<ResourceReincarnation> getResource(size_t frameIndex)const = 0;
};


class DescriptorSet{
    struct Binding:DescriptorLayout{
        std::shared_ptr<Resource> resource;
        struct Frame{
            std::shared_ptr<ResourceReincarnation> reincarnation;
        };
        std::vector<Frame> frames;

        Binding(const DescriptorLayout& dsLayout);

        DescriptorInfo descriptorInfo;
    };
    std::vector<std::vector<Binding>> bindings;
    std::map<size_t, size_t> mappingID2Index;

    class Swapchain* swapchain = nullptr;
public:
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;


    void create(Device& device,class Window& window,DescriptorSetLayout& dsl,std::vector<DescriptorLayout> dsArray);
    void bind(Device& device,vk::raii::CommandBuffer& commandBuffer,Window& window, class Pipeline& pipeline,uint32_t firstSet = 0);

    void setResource(std::shared_ptr<Resource> resource,size_t binding,size_t arrayIndex = 0);

    ~DescriptorSet();
};

