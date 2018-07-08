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
    
}