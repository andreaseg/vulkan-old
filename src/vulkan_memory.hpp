#ifndef VULKAN_MEMORY_HPP
#define VULKAN_MEMORY_HPP

#include "includes.hpp"
#include <tuple>
#include <memory>
#include <map>
#include <vector>

namespace vk_mem {

    static const vk::DeviceSize MEMORY_BLOCK_SIZE = 256 * 1024 * 1024;
    static const vk::DeviceSize MEMORY_SUBBLOCK_SIZE = 16 * 1024;

    struct BufferContainer {
        vk::Buffer internal_buffer;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        bool operator < (const BufferContainer& str) const;
    };

    struct BufferHandle {
        uint32_t type;
        vk::DeviceSize offset;
    };

    struct MemoryBlock {
        vk::DeviceMemory memory;
        vk::DeviceSize size;
        std::vector<BufferContainer> buffers;
        MemoryBlock *next;
    };

    class Manager {
        public:
        Manager() {};
        Manager(vk::PhysicalDevice *p_physical_device, vk::Device *p_device, vk::Queue *p_queue, vk::CommandPool *p_command_pool);

        BufferHandle create_transfer_buffer(const vk::DeviceSize size);
        BufferHandle create_vertex_buffer(const vk::DeviceSize size);
        BufferHandle create_index_buffer(const vk::DeviceSize size);
        void copy_buffer(BufferHandle &src, BufferHandle &dst);
        void free(const BufferHandle &handle);
        BufferContainer* get_buffer(const BufferHandle &handle);
        void* mapMemory(const BufferHandle &handle, const vk::MemoryMapFlags flags = vk::MemoryMapFlags());
        void unmapMemory(const BufferHandle &handle);

        void destroy();

        private:

        std::map<uint32_t, MemoryBlock> memory_blocks;

        vk::PhysicalDevice *p_physical_device;
        vk::Device *p_device;
        vk::Queue *p_queue;
        vk::CommandPool *p_command_pool;

        BufferHandle create_buffer(const uint32_t size, const vk::BufferUsageFlags usage_flags, const vk::MemoryPropertyFlags properties);
        vk::DeviceMemory *get_memory(const BufferHandle &handle);
        
    };

}

#endif