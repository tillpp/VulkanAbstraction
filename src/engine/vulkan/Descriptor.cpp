#include "Descriptor.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "engine/vulkan/Pipeline.hpp"
#include "engine/vulkan/Window.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

DescriptorSet::Binding::Binding(const DescriptorLayout& dsLayout):DescriptorLayout(dsLayout){
    for (int i = 0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        frames.push_back({});
    }
}

//TODO: assert that dsArray[_].descriptorCount is equal dsl...descriptorCounts. Exception variableDescriptorCount, where i can also be smaller. 
void DescriptorSet::create(Device& device,Window& window,DescriptorSetLayout& dsl,std::vector<DescriptorLayout> dsArray){
    this->swapchain = &window.swapchain;
    //pool creation
    {
        std::map<vk::DescriptorType,uint32_t> poolsizes;
        for(auto&ds:dsArray){
            if(poolsizes.find(ds.descriptorType) == poolsizes.end())
                poolsizes[ds.descriptorType] = 0;
            poolsizes[ds.descriptorType] += ds.descriptorCount;
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
        std::vector<vk::DescriptorSetLayout> layouts        (MAX_FRAMES_IN_FLIGHT, *dsl.descriptorSetLayout);

        // descriptorIndexing
        uint32_t highestBinding = dsArray.size()? dsArray[0].binding:0;
        for(auto i = 0;i<dsArray.size(); i++){
            if(dsArray[i].binding > highestBinding)
                highestBinding = dsArray[i].binding;
        }
        std::optional<size_t> variableCountIndex = std::nullopt;
        for(auto i = 0;i<dsArray.size(); i++){
            if(dsArray[i].variableCount){
                variableCountIndex = i;

                if(dsArray[i].binding != highestBinding)
                    assert(0 && "only last bind can be variableCount descriptor");
            }
        }
        std::vector<uint32_t>                                descriptorCount(MAX_FRAMES_IN_FLIGHT,
            variableCountIndex.has_value() ? dsArray[variableCountIndex.value()].descriptorCount : 0);
        vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCount;
        variableDescriptorCount.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        variableDescriptorCount.pDescriptorCounts  = descriptorCount.data();

            
        vk::DescriptorSetAllocateInfo allocInfo{ 
                .descriptorPool     = descriptorPool, 
                .descriptorSetCount = static_cast<uint32_t>(layouts.size()), 
                .pSetLayouts        = layouts.data() 
        };
        if(variableCountIndex.has_value()){
            allocInfo.pNext         = variableDescriptorCount;            
        }

        descriptorSets.clear();
        descriptorSets = device.device.allocateDescriptorSets(allocInfo);
    }
    for (auto& dsLayout : dsArray) {
        mappingID2Index[dsLayout.binding] = bindings.size();
        bindings.emplace_back(dsLayout.descriptorCount,dsLayout);
    }
}
void DescriptorSet::bind(Device& device,vk::raii::CommandBuffer& commandBuffer,Window& window, Pipeline& pipeline,uint32_t firstSet ){
    //Updating reincarnations:

    auto fi = window.swapchain.getFrameIndex();

    std::vector<vk::WriteDescriptorSet>    descriptorWrites;
    std::vector<vk::DescriptorImageInfo*>  imageInfos;
    std::vector<vk::DescriptorBufferInfo*> bufferInfos;

    for(int bindIndex = 0;bindIndex < bindings.size(); bindIndex++){
        auto& bind          = bindings[bindIndex];
        for (uint32_t i = 0; i < bind.size() ; i++) {
            auto& element = bind[i];
            auto& reincarnation = element.frames[fi].reincarnation;
            auto& resource      = element.resource;

            if(element.variableCount){
                if(!resource) continue;
            }
            else assert(resource);

            // missmatch?
            if(reincarnation != resource->getResource(fi)){
                reincarnation = resource->getResource(fi);
                vk::WriteDescriptorSet wds{ 
                    .dstSet          = descriptorSets[fi], 
                    .dstBinding      = element.binding, 
                    .dstArrayElement = i, 
                    .descriptorCount = 1,
                    .descriptorType  = element.descriptorType
                };
                auto& descriptorInfo = element.descriptorInfo;
                descriptorInfo = reincarnation->getDescriptorInfo();

                if(descriptorInfo.type == DescriptorInfo::BUFFER){
                    auto dbi = new vk::DescriptorBufferInfo[1]{
                        descriptorInfo.bufferInfo
                    };
                    wds.pBufferInfo = dbi;
                    bufferInfos.push_back(dbi);
                }else if(descriptorInfo.type == DescriptorInfo::IMAGE){
                    auto dii = new vk::DescriptorImageInfo[1]{
                        descriptorInfo.imageInfo
                    };
                    wds.pImageInfo = dii;
                    imageInfos.push_back(dii);
                }
                descriptorWrites.push_back(wds);
            }
        }
    }
    if(descriptorWrites.size())
        device.device.updateDescriptorSets(descriptorWrites, {});
    for(auto x:imageInfos)
        delete[] x;
    for(auto x:bufferInfos)
        delete[] x;

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipelineLayout, firstSet, *descriptorSets[fi], nullptr);
}
void DescriptorSet::setResource(std::shared_ptr<Resource> resource,size_t binding,size_t arrayIndex){
    bindings[mappingID2Index[binding]][arrayIndex].resource = resource;
}
DescriptorSet::~DescriptorSet(){
    if(swapchain){
        auto& trashCan = swapchain->trashCan;
        for(auto& array:bindings){
            for(auto& bind:array){
                for(auto& frame:bind.frames){   
                    trashCan.trash(frame.reincarnation);
                }
            }
        }
        trashCan.trash(std::move(descriptorPool));
        for(auto& ds:descriptorSets){
            trashCan.trash(std::move(ds));
        }
    }
}