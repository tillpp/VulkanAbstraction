#pragma once
#include "engine/vulkan/CommandBuffer.hpp"
#include "engine/vulkan/Swapchain.hpp"
#include "vulkan/vulkan.hpp"

//NOTE: there are only 4096 max memory allocations, split bigger buffers into smaller ones with offset. (TODO: custom allocator)
//NOTE: IndexBuffer and VertexBuffer in one, is more cache friendly.
//TODO: learn more about "aliasing" in Vulkan

class Buffer{
    Swapchain* swapchain = nullptr;
public:
    vk::raii::DeviceMemory bufferMemory = nullptr;
    vk::raii::Buffer       buffer       = nullptr;

    ~Buffer();

    static uint32_t findMemoryType(Device& device,uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    static void copyBuffer(CommandPool& commandPool,Buffer& srcBuffer, Buffer& dstBuffer, vk::DeviceSize size);

    //TODO: Buffer shouldnt rely on window
    void create(class Window& window,Device& device,vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    void createAndUpload(class Window& window,const void* ptr,size_t size,vk::Flags<vk::BufferUsageFlagBits> usage);

    void bindAsVertexBuffers(CommandBuffer& cb,vk::DeviceSize offset);
};