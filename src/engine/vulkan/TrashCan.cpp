#include "TrashCan.hpp"
#include "Swapchain.hpp"

void TrashCan::TrashLayer::clear(){
    commandBuffers.clear();
    pipelines.clear();
    pipelineLayouts.clear();
    descriptorSets.clear();
    descriptorPools.clear();
    buffers.clear();
    deviceMemorys.clear();
    reincarnations.clear();
}
TrashCan::TrashCan(Swapchain& swapchain):frameIndex(swapchain.getFrameIndex()){
}

TrashCan::~TrashCan(){
    clearAll();
}
void TrashCan::clearAll(){
    for (auto& tl : trashLayer) {
        tl.clear();
    }
}
void TrashCan::clear(){
    trashLayer[frameIndex].clear();
}

uint32_t TrashCan::getFrameIndex()const{
    return frameIndex;
}

    

void TrashCan::trash(vk::raii::Buffer buffer,vk::raii::DeviceMemory deviceMemory){
    trashLayer[getFrameIndex()].buffers.push_back(std::move(buffer));
    trashLayer[getFrameIndex()].deviceMemorys.push_back(std::move(deviceMemory));
}
void TrashCan::trash(vk::raii::PipelineLayout pipelineLayout,vk::raii::Pipeline pipeline){
    trashLayer[getFrameIndex()].pipelineLayouts.push_back(std::move(pipelineLayout));
    trashLayer[getFrameIndex()].pipelines.push_back(std::move(pipeline));
}
void TrashCan::trash(vk::raii::DescriptorPool descriptorPool){
    trashLayer[getFrameIndex()].descriptorPools.push_back(std::move(descriptorPool));
}
void TrashCan::trash(vk::raii::DescriptorSet descriptorSet){
    trashLayer[getFrameIndex()].descriptorSets.push_back(std::move(descriptorSet));
}

void TrashCan::trash(class CommandBuffer& cb){
    trashLayer[getFrameIndex()].commandBuffers.push_back(std::move(cb.commandBuffer));
}
void TrashCan::trash(std::shared_ptr<class ResourceReincarnation> reincarnation){
    trashLayer[getFrameIndex()].reincarnations.push_back(reincarnation);
}