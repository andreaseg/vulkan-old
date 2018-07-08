#ifndef VULKAN_MEMORY_HPP
#define VULKAN_MEMORY_HPP

#include "includes.hpp"
#include <tuple>
#include <memory>

namespace vk_mem {

    class Manager {
        public:
        Manager() {};
        Manager(vk::PhysicalDevice *p_physical_device, vk::Device *p_device, vk::Queue *p_queue, vk::CommandPool *p_command_pool);

        std::tuple<vk::Buffer, vk::DeviceMemory> create_transfer_buffer(vk::DeviceSize size);
        std::tuple<vk::Buffer, vk::DeviceMemory> create_vertex_buffer(vk::DeviceSize size);
        void copy_buffer(vk::Buffer &src, vk::Buffer &dst, vk::DeviceSize size);

        private:

        vk::PhysicalDevice *p_physical_device;
        vk::Device *p_device;
        vk::Queue *p_queue;
        vk::CommandPool *p_command_pool;
    };

}

#endif