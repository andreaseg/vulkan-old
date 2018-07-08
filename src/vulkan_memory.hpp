#ifndef VULKAN_MEMORY_HPP
#define VULKAN_MEMORY_HPP

#include "includes.hpp"
#include <tuple>

namespace vk_mem {
    std::tuple<vk::Buffer, vk::DeviceMemory> create_buffer(const vk::PhysicalDevice physical_device, const vk::Device &device, const uint32_t size, const vk::BufferUsageFlags usage_flags, const vk::MemoryPropertyFlags properties);
    void copy_buffer(const vk::Device &device, const vk::Queue &queue, const vk::CommandPool &command_pool, vk::Buffer &source, vk::Buffer &dest, vk::DeviceSize size);
}

#endif