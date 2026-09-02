#include "Descriptor.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/Pipeline.hpp"
#include "engine/vulkan/Window.hpp"
#include <cassert>
#include <vector>

DescriptorSet::Binding::Binding(const DescriptorLayout& dsLayout):DescriptorLayout(dsLayout){
    for (int i = 0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        frames.push_back({});
    }
}

void DescriptorSet::create(Device& device,Window& window,DescriptorSetLayout& dsl,std::vector<DescriptorLayout> dsArray){
    this->swapchain = &window.swapchain;
    //pool creation
    {
        std::map<vk::DescriptorType,uint32_t> poolsizes;
        for(auto&ds:dsArray){
            if(poolsizes.find(ds.descriptorType) == poolsizes.end())
                poolsizes[ds.descriptorType] = 0;
            poolsizes[ds.descriptorType]++;
        }

        std::vector<vk::DescriptorPoolSize> poolSize;
        for(auto& pair:poolsizes){
            poolSize.push_back(
                vk::DescriptorPoolSize{
                    .type = pair.first,
                    .descriptorCount = MAX_FRAMES_IN_FLIGHT*pair.second,
                }
            );
        }
        vk::DescriptorPoolCreateInfo poolInfo{ 
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount = (uint32_t)poolSize.size(),
            .pPoolSizes = poolSize.data(),
        };
        descriptorPool = vk::raii::DescriptorPool(device.device, poolInfo);
    }
    {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *dsl.descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo{ 
            .descriptorPool = descriptorPool, 
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()), 
            .pSetLayouts = layouts.data() 
        };

        descriptorSets.clear();
        descriptorSets = device.device.allocateDescriptorSets(allocInfo);
    }
    for (auto& dsLayout : dsArray) {
        mappingID2Index[dsLayout.binding] = bindings.size();
        bindings.emplace_back(dsLayout);
    }
}
void DescriptorSet::bind(Device& device,vk::raii::CommandBuffer& commandBuffer,Window& window, Pipeline& pipeline,uint32_t firstSet ){
    //Updating reincarnations:

    auto fi = window.swapchain.getFrameIndex();

    std::vector<vk::WriteDescriptorSet> descriptorWrites;
    DescriptorInfo descriptorInfos[bindings.size()];

    for(int bindIndex = 0;bindIndex < bindings.size(); bindIndex++){
        auto& bind          = bindings[bindIndex];
        auto& reincarnation = bind.frames[fi].reincarnation;
        auto& resource      = bind.resource;
        
        assert(resource);

        // missmatch?
        if(reincarnation != resource->getResource(fi)){
            reincarnation = resource->getResource(fi);
            vk::WriteDescriptorSet wds{ 
                .dstSet = descriptorSets[fi], 
                .dstBinding = bind.binding, 
                .dstArrayElement = 0, 
                .descriptorCount = 1,
                .descriptorType = bind.descriptorType
            };
            auto& descriptorInfo =descriptorInfos[bindIndex];
            descriptorInfo = reincarnation->getDescriptorInfo();

            if(descriptorInfo.type == DescriptorInfo::BUFFER){
                wds.pBufferInfo = &descriptorInfo.bufferInfo;
            }else if(descriptorInfo.type == DescriptorInfo::IMAGE){
                wds.pImageInfo = &descriptorInfo.imageInfo;
            }
            descriptorWrites.push_back(wds);
        }
    }
    if(descriptorWrites.size())
        device.device.updateDescriptorSets(descriptorWrites, {});

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipelineLayout, firstSet, *descriptorSets[fi], nullptr);
}
void DescriptorSet::setResource(size_t binding,std::shared_ptr<Resource> resource){
    bindings[mappingID2Index[binding]].resource = resource;
}
DescriptorSet::~DescriptorSet(){
    if(swapchain){
        auto& trashCan = swapchain->trashCan;
        for(auto& bind:bindings){
            for(auto& frame:bind.frames){
                trashCan.trash(frame.reincarnation);
            }
        }
        trashCan.trash(std::move(descriptorPool));
        for(auto& ds:descriptorSets){
            trashCan.trash(std::move(ds));
        }
    }
}