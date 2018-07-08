#include "vulkan_memory.hpp"
#include <iostream>

namespace vk_mem {
    uint32_t find_memory_type(const vk::PhysicalDevice &physical_device, const vk::MemoryRequirements &mem_req, const vk::MemoryPropertyFlags property_flags) {
        auto properties = physical_device.getMemoryProperties();

        for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
            if ((mem_req.memoryTypeBits & (1 << i))
                && ((properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)) {
                    return i;
            }
        }
        throw("Unable to allocate memory");
    }

    std::tuple<vk::Buffer, vk::DeviceMemory> create_buffer(const vk::PhysicalDevice physical_device, const vk::Device &device, const uint32_t size, const vk::BufferUsageFlags usage_flags, const vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo create_info(
            vk::BufferCreateFlags(),
            size,
            usage_flags,
            vk::SharingMode::eExclusive
        );

        vk::Buffer buffer = device.createBuffer(create_info);

        vk::MemoryRequirements mem_reqs = device.getBufferMemoryRequirements(buffer);

        uint32_t memory_type = find_memory_type(physical_device, mem_reqs, properties);

        vk::MemoryAllocateInfo alloc_info(mem_reqs.size, memory_type);

        vk::DeviceMemory memory = device.allocateMemory(alloc_info);

        device.getMemoryCommitment(memory);

        device.bindBufferMemory(buffer, memory, 0 /* offset */);

        return {buffer, memory};
    }

    Manager::Manager(vk::PhysicalDevice *p_physical_device, vk::Device *p_device, vk::Queue *p_queue, vk::CommandPool *p_command_pool)
        : p_physical_device(p_physical_device), p_device(p_device), p_queue(p_queue), p_command_pool(p_command_pool) {}

    std::tuple<vk::Buffer, vk::DeviceMemory> Manager::create_transfer_buffer(vk::DeviceSize size) {
        return create_buffer(
            *p_physical_device, *p_device, size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    std::tuple<vk::Buffer, vk::DeviceMemory> Manager::create_vertex_buffer(vk::DeviceSize size) {
        return create_buffer(
            *p_physical_device, *p_device, size,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
    }

    std::tuple<vk::Buffer, vk::DeviceMemory> Manager::create_index_buffer(vk::DeviceSize size) {
        return create_buffer(
            *p_physical_device, *p_device, size,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
    }

    void Manager::copy_buffer(vk::Buffer &src, vk::Buffer &dst, vk::DeviceSize size) {
        vk::CommandBufferAllocateInfo alloc_info(
            *p_command_pool,
            vk::CommandBufferLevel::ePrimary,
            1
        );

        vk::CommandBuffer command_buffer = p_device->allocateCommandBuffers(alloc_info)[0];

        vk::CommandBufferBeginInfo begin_info(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        );

        command_buffer.begin(begin_info);
        {
            vk::BufferCopy copy_region(
                0,      // Source offset
                0,      // Destination offset
                size    // Size
            );
            
            command_buffer.copyBuffer(src, dst, copy_region);
        }
        command_buffer.end();

        vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &command_buffer, 0, nullptr);

        p_queue->submit(submit_info, nullptr);
        p_queue->waitIdle();
        p_device->freeCommandBuffers(*p_command_pool, command_buffer);
    }
    
}