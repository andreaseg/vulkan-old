#include "vulkan_memory.hpp"
#include <iostream>
#include <cstdlib>

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

    template<typename T>
    inline T integer_step(T val, T step) {
        return ((val + step - 1) / step) * step;
    }

    std::ostream& operator<< (std::ostream& stream, const BufferHandle& handle) {
        stream << "BufferHandle<";
        bool multiple_flags = false;
        if ((handle.type & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            if (multiple_flags) stream << "+";
            stream << "DeviceLocal";
            multiple_flags = true;
        }
        if ((handle.type & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
            if (multiple_flags) stream << "+";
            stream << "HostCached";
            multiple_flags = true;
        }
        if ((handle.type & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
            if (multiple_flags) stream << "+";
            stream << "HostCoherent";
            multiple_flags = true;
        }
        if ((handle.type & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
            if (multiple_flags) stream << "+";
            stream << "HostVisible";
            multiple_flags = true;
        }
        if ((handle.type & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0) {
            if (multiple_flags) stream << "+";
            stream << "Lazy";
            multiple_flags = true;
        }
        if ((handle.type & VK_MEMORY_PROPERTY_PROTECTED_BIT) != 0) {
            if (multiple_flags) stream << "+";
            stream << "Protected";
            multiple_flags = true;
        }
        stream << ">[" << handle.offset << "]";
        return stream;
    }

    bool BufferContainer::operator < (const BufferContainer& other) const {
        return this->offset < other.offset;
    }

    BufferHandle Manager::create_buffer(const uint32_t size, const vk::BufferUsageFlags usage_flags, const vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo create_info(
            vk::BufferCreateFlags(),
            size,
            usage_flags,
            vk::SharingMode::eExclusive
        );

        vk::Buffer buffer = p_device->createBuffer(create_info);

        vk::MemoryRequirements mem_reqs = p_device->getBufferMemoryRequirements(buffer);

        uint32_t memory_type = find_memory_type(*p_physical_device, mem_reqs, properties);

        MemoryBlock *mem_block = &memory_blocks[memory_type];

        vk::DeviceSize mem_block_index = 0;

        while(true) {
            if (!mem_block->memory) {
                vk::MemoryAllocateInfo alloc_info(MEMORY_BLOCK_SIZE, memory_type);
                try {
                    mem_block->memory = p_device->allocateMemory(alloc_info);
                } catch (...) {
                    std::cerr << "Unable to allocate memory" << std::endl;
                    break;
                }
                std::cout << "Allocating memory of type " << memory_type << " and size " << MEMORY_BLOCK_SIZE << std::endl;
            }

            vk::DeviceSize last_buffer_end = 0;

            for(auto &[key, val] : mem_block->buffers) {
                
                vk::DeviceSize next_buffer_start = integer_step(val.offset, MEMORY_SUBBLOCK_SIZE);
                if (integer_step(next_buffer_start - last_buffer_end, MEMORY_SUBBLOCK_SIZE) >= mem_reqs.size) {
                    BufferContainer container;
                    container.internal_buffer = buffer;
                    container.offset = last_buffer_end;
                    container.size = mem_reqs.size;
                    mem_block->buffers[last_buffer_end] = container;
                    p_device->bindBufferMemory(mem_block->buffers[last_buffer_end].internal_buffer, mem_block->memory, last_buffer_end);

                    BufferHandle handle;
                    handle.type = memory_type;
                    handle.offset = last_buffer_end + mem_block_index * MEMORY_BLOCK_SIZE;
                    std::cout << "Bound " << handle << std::endl;
                    return handle;
                }
                last_buffer_end = integer_step(next_buffer_start + val.size, MEMORY_SUBBLOCK_SIZE);
            }

            if (integer_step(mem_reqs.size + last_buffer_end, MEMORY_SUBBLOCK_SIZE) < MEMORY_BLOCK_SIZE) {
                BufferContainer container;
                container.internal_buffer = buffer;
                container.offset = last_buffer_end + mem_block_index * MEMORY_BLOCK_SIZE;
                container.size = mem_reqs.size;
                mem_block->buffers[container.offset] = container;
                p_device->bindBufferMemory(mem_block->buffers[container.offset].internal_buffer, mem_block->memory, last_buffer_end);

                BufferHandle handle;
                handle.type = memory_type;
                handle.offset = last_buffer_end;
                std::cout << "Bound " << handle << std::endl;
                return handle;
            }

            if(mem_block->next == nullptr) {
                mem_block->next = new MemoryBlock();
            }

            mem_block = mem_block->next;
            ++mem_block_index;
        }

        throw("Unable to allocate buffer");
    }

    void Manager::free(const BufferHandle &handle) {
        auto *mem_block = &memory_blocks[handle.type];

        for (size_t i = 0; i < handle.offset % MEMORY_BLOCK_SIZE; ) {
            mem_block = mem_block->next;
        }

        auto it = mem_block->buffers.find(handle.offset);
        if (it != mem_block->buffers.end()) {
            p_device->destroyBuffer(it->second.internal_buffer);
            mem_block->buffers.erase (it);
            std::cout << "Freed " << handle << std::endl;
            return;
        }

        throw("Unable to locate buffer");
    }

    BufferContainer* Manager::get_buffer(const BufferHandle &handle) {
        auto *mem_block = &memory_blocks[handle.type];

        for (size_t i = 0; i < handle.offset / MEMORY_BLOCK_SIZE; ) {
            mem_block = mem_block->next;
        }

        auto it = mem_block->buffers.find(handle.offset % MEMORY_BLOCK_SIZE);
        if (it != mem_block->buffers.end()) {
            return &it->second;
        }

        throw("Unable to locate buffer");
    }

    vk::DeviceMemory* Manager::get_memory(const BufferHandle &handle) {
        auto *mem_block = &memory_blocks[handle.type];

        for (size_t i = 0; i < handle.offset / MEMORY_BLOCK_SIZE; ) {
            mem_block = mem_block->next;
        }

        return &mem_block->memory;
    }

    Manager::Manager(vk::PhysicalDevice *p_physical_device, vk::Device *p_device, vk::Queue *p_queue, vk::CommandPool *p_command_pool)
        : p_physical_device(p_physical_device), p_device(p_device), p_queue(p_queue), p_command_pool(p_command_pool) {}

    BufferHandle Manager::create_transfer_buffer(const vk::DeviceSize size) {
        return create_buffer(
            size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    BufferHandle Manager::create_vertex_buffer(const vk::DeviceSize size) {
        return create_buffer(
            size,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
    }

    BufferHandle Manager::create_index_buffer(const vk::DeviceSize size) {
        return create_buffer(
            size,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
    }

    void Manager::copy_buffer(BufferHandle &src, BufferHandle &dst) {
        BufferContainer *p_src = get_buffer(src);
        BufferContainer *p_dst = get_buffer(dst);

        if (p_dst->size < p_src->size) {
            throw("Error: Attempting to copy to a undersized buffer");
        }

        std::cout << "Copying from " << src << " to " << dst << std::endl;

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
                p_src->offset,      // Source offset
                p_dst->offset,      // Destination offset
                p_src->size        // Size
            );
            
            command_buffer.copyBuffer(p_src->internal_buffer, p_dst->internal_buffer, copy_region);
        }
        command_buffer.end();

        vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &command_buffer, 0, nullptr);

        p_queue->submit(submit_info, nullptr);
        p_queue->waitIdle();
        p_device->freeCommandBuffers(*p_command_pool, command_buffer);
    }

    void* Manager::mapMemory(const BufferHandle &handle, const vk::MemoryMapFlags flags) {
        BufferContainer *p_con = get_buffer(handle);
        std::cout << "Mapping " << handle << " to memory" << std::endl;
        return p_device->mapMemory(*get_memory(handle), p_con->offset, p_con->size, flags);
    }

    void Manager::unmapMemory(const BufferHandle &handle) {
        std::cout << "Unmapping memory" << std::endl;
        p_device->unmapMemory(*get_memory(handle));
    }

    void destroy_recursive(const vk::Device &device, MemoryBlock &block) {
        for (auto &[key, val] : block.buffers) {
            device.destroyBuffer(val.internal_buffer);
        }
        device.free(block.memory);

        if (block.next != nullptr) {
            destroy_recursive(device, *block.next);
        }

        delete block.next;
    }

    void Manager::destroy() {
        for (auto &[key, val] : memory_blocks) {
            destroy_recursive(*p_device, val);
        }
    }
    
}