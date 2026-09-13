#pragma once
#include "engine/vulkan/common.hpp"// IWYU pragma: keep
#include "engine/vulkan/DescriptorLayout.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
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
    virtual ~ResourceReincarnation()=default;
};
struct ResourceUpdate{
    uint32_t binding;
    uint32_t arrayIndex;

    bool operator<(const ResourceUpdate& rhs) const{
        if(binding != rhs.binding)
            return binding < rhs.binding;
        return arrayIndex < rhs.arrayIndex;
    }
};
struct Resource{
    virtual std::shared_ptr<ResourceReincarnation> getResource(size_t frameIndex)const = 0;
protected:
    void notifyDescriptorSet();
private:
    std::map<class DescriptorSet*, std::set<ResourceUpdate>> descriptorSets;
    friend class DescriptorSet;
    void registerDescriptorSet(class DescriptorSet* descriptorSet, uint32_t binding, uint32_t arrayIndex);
    void deregisterDescriptorSet(class DescriptorSet* descriptorSet,uint32_t binding, uint32_t arrayIndex);
    void deregisterDescriptorSetEverything(class DescriptorSet* descriptorSet);
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

    
    std::vector<std::set<ResourceUpdate>> updates;
public:
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;


    void create(Device& device,class Window& window,DescriptorSetLayout& dsl,std::vector<DescriptorLayout> dsArray);
    void bind(Device& device,vk::raii::CommandBuffer& commandBuffer,Window& window, class Pipeline& pipeline,uint32_t firstSet);

    void setResource(std::shared_ptr<Resource> resource,size_t binding,size_t arrayIndex = 0);
    
    void requestReincarnationUpdate(size_t frameIndex, uint32_t binding, uint32_t arrayIndex);
    void update(Device& device,Window& window);
    
    virtual ~DescriptorSet();
};

