#ifndef VULKAN_MEMORY_HPP
#define VULKAN_MEMORY_HPP

#include "includes.hpp"
#include <tuple>
#include <memory>
#include <map>
#include <vector>

namespace vk_mem {

    static const vk::DeviceSize MEMORY_BLOCK_SIZE = 64 * 1024 * 1024;
    static const vk::DeviceSize MEMORY_SUBBLOCK_SIZE = 1024;

    struct ImageContainer {
        vk::Image internal_image;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        bool operator < (const ImageContainer& str) const;
        operator vk::Image() const;
    };

    struct BufferContainer {
        vk::Buffer internal_buffer;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        bool operator < (const BufferContainer& str) const;
        operator vk::Buffer() const;
    };

    struct ImageHandle {
        uint32_t type;
        vk::DeviceSize offset;

        friend std::ostream& operator<< (std::ostream& stream, const ImageHandle& handle);
    };

    struct BufferHandle {
        uint32_t type;
        vk::DeviceSize offset;

        friend std::ostream& operator<< (std::ostream& stream, const BufferHandle& handle);
    };

    struct MemoryBlock {
        vk::DeviceMemory memory;
        vk::DeviceSize size;
        std::map<vk::DeviceSize, BufferContainer> buffers;
        MemoryBlock *next;
    };

    class Manager {
        public:
        Manager() {};
        Manager(vk::PhysicalDevice *p_physical_device, vk::Device *p_device, vk::Queue *p_queue, vk::CommandPool *p_command_pool);

        uint32_t find_memory_type(const vk::MemoryRequirements &mem_req, const vk::MemoryPropertyFlags property_flags);

        BufferHandle create_transfer_buffer(const vk::DeviceSize size);
        BufferHandle create_vertex_buffer(const vk::DeviceSize size);
        BufferHandle create_index_buffer(const vk::DeviceSize size);
        BufferHandle create_uniform_buffer(const vk::DeviceSize size);
        void copy_buffer(BufferHandle &src, BufferHandle &dst);
        void copy_buffer(BufferHandle &src_handle, vk::Image &dst_image, uint32_t width, uint32_t height, const vk::Format &format, const vk::ImageLayout &old_layout);
        void free(const BufferHandle &handle);
        BufferContainer get_buffer(const BufferHandle &handle);
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
        vk::DeviceMemory get_memory(const BufferHandle &handle);

        vk::CommandBuffer begin_one_time_command();
        void end_one_time_command(vk::CommandBuffer &command_buffer);
        void layout_transition(vk::Image image, const vk::Format &format, const vk::ImageLayout &old_layout, const vk::ImageLayout &new_layout);
        
    };

}

#endif