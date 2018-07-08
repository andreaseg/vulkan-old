#include "vulkan_memory.hpp"

namespace vk_mem {
    uint32_t find_memory (const vk::PhysicalDevice &physical_device, const vk::MemoryRequirements &mem_req, const vk::MemoryPropertyFlags property_flags) {
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

        vk::MemoryAllocateInfo alloc_info(mem_reqs.size, find_memory(physical_device, mem_reqs, properties));

        vk::DeviceMemory memory = device.allocateMemory(alloc_info);

        device.bindBufferMemory(buffer, memory, 0 /* offset */);

        return {buffer, memory};
    }

    void copy_buffer(const vk::Device &device, const vk::Queue &queue, const vk::CommandPool &command_pool, vk::Buffer &source, vk::Buffer &dest, vk::DeviceSize size) {
        vk::CommandBufferAllocateInfo alloc_info(
            command_pool,
            vk::CommandBufferLevel::ePrimary,
            1
        );

        vk::CommandBuffer command_buffer = device.allocateCommandBuffers(alloc_info)[0];

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
            
            command_buffer.copyBuffer(source, dest, copy_region);
        }
        command_buffer.end();

        vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &command_buffer, 0, nullptr);

        queue.submit(submit_info, nullptr);
        queue.waitIdle();
        device.freeCommandBuffers(command_pool, command_buffer);
    }
    
}