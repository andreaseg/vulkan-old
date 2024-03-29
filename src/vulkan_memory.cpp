#include "vulkan_memory.hpp"
#include <iostream>
#include <cstdlib>

namespace vk_mem {
    uint32_t Manager::find_memory_type(const vk::MemoryRequirements &mem_req, const vk::MemoryPropertyFlags property_flags) {
        auto properties = p_physical_device->getMemoryProperties();

        for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
            if ((mem_req.memoryTypeBits & (1 << i))
                && ((properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)) {
                    return i;
            }
        }
        throw std::runtime_error("Unable to allocate memory");
    }

    template<typename T>
    inline T integer_step(T val, T step) {
        return ((val + step - 1) / step) * step;
    }

    std::string memory_type_to_string(uint32_t type) {
        std::string ret = "";
        bool multiple_flags = false;
        if ((type & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            ret += "DeviceLocal";
            multiple_flags = true;
        }
        if ((type & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
            if (multiple_flags) ret += "+";
            ret += "HostCached";
            multiple_flags = true;
        }
        if ((type & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
            if (multiple_flags) ret += "+";
            ret += "HostCoherent";
            multiple_flags = true;
        }
        if ((type & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
            if (multiple_flags) ret += "+";
            ret += "HostVisible";
            multiple_flags = true;
        }
        if ((type & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0) {
            if (multiple_flags) ret += "+";
            ret += "Lazy";
            multiple_flags = true;
        }
        if ((type & VK_MEMORY_PROPERTY_PROTECTED_BIT) != 0) {
            if (multiple_flags) ret += "+";
            ret += "Protected";
            multiple_flags = true;
        }
        return ret;
    }

    std::ostream& operator<< (std::ostream& stream, const BufferHandle& handle) {
        stream << "Buffer<";
        stream << memory_type_to_string(handle.type);
        stream << ">[" << handle.offset / MEMORY_SUBBLOCK_SIZE << "]";
        return stream;
    }

    std::ostream& operator<< (std::ostream& stream, const ImageHandle& handle) {
        stream << "Image<";
        stream << memory_type_to_string(handle.type);
        stream << ">[" << handle.offset / MEMORY_SUBBLOCK_SIZE << "]";
        return stream;
    }

    bool BufferContainer::operator < (const BufferContainer& other) const {
        return this->offset < other.offset;
    }

    BufferContainer::operator vk::Buffer() const {
        return this->internal_buffer;
    }

    bool ImageContainer::operator < (const ImageContainer& other) const {
        return this->offset < other.offset;
    }

    ImageContainer::operator vk::Image() const {
        return this->internal_image;
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

        uint32_t memory_type = find_memory_type(mem_reqs, properties);

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
                std::cout << "Allocating memory of type <" << memory_type_to_string(memory_type) << "> and size " << MEMORY_BLOCK_SIZE / 1048576.0f << "MB" << std::endl;
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

        throw std::runtime_error("Unable to allocate buffer");
    }

    void Manager::free(const BufferHandle &handle) {
        auto *mem_block = &memory_blocks[handle.type];

        for (size_t i = 0; i < handle.offset / MEMORY_BLOCK_SIZE; ) {
            mem_block = mem_block->next;
        }

        auto it = mem_block->buffers.find(handle.offset);
        if (it != mem_block->buffers.end()) {
            p_device->destroyBuffer(it->second.internal_buffer);
            mem_block->buffers.erase (it);
            std::cout << "Freed " << handle << std::endl;
            return;
        }

        std::cerr << "Unable to locate buffer" << std::endl;
        throw std::runtime_error("Unable to locate buffer");
    }

    BufferContainer Manager::get_buffer(const BufferHandle &handle) {

        if (handle.type == 0) {
            throw std::runtime_error("Null handle provided");
        }

        auto *mem_block = &memory_blocks[handle.type];

        for (size_t i = 0; i < handle.offset / MEMORY_BLOCK_SIZE; ) {
            mem_block = mem_block->next;
        }

        auto it = mem_block->buffers.find(handle.offset % MEMORY_BLOCK_SIZE);
        if (it != mem_block->buffers.end()) {
            return it->second;
        }

        throw std::runtime_error("Unable to locate buffer");
    }

    vk::DeviceMemory Manager::get_memory(const BufferHandle &handle) {
        auto *mem_block = &memory_blocks[handle.type];

        for (size_t i = 0; i < handle.offset / MEMORY_BLOCK_SIZE; ) {
            mem_block = mem_block->next;
        }

        return mem_block->memory;
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

    BufferHandle Manager::create_uniform_buffer(const vk::DeviceSize size) {
        return create_buffer(
            size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    vk::CommandBuffer Manager::begin_one_time_command() {
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

        return command_buffer;
    }

    void Manager::end_one_time_command(vk::CommandBuffer &command_buffer) {
        command_buffer.end();

        vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &command_buffer, 0, nullptr);

        p_queue->submit(submit_info, nullptr);
        p_queue->waitIdle();
        p_device->freeCommandBuffers(*p_command_pool, command_buffer);
    }

    void Manager::copy_buffer(BufferHandle &src_handle, BufferHandle &dst_handle) {
        BufferContainer src = get_buffer(src_handle);
        BufferContainer dst = get_buffer(dst_handle);

        if (dst.size < src.size) {
            throw std::runtime_error("Error: Attempting to copy to a undersized buffer");
        }

        std::cout << "Copying from " << src_handle << " to " << dst_handle << std::endl;

        auto command_buffer = begin_one_time_command();
        
        vk::BufferCopy copy_region(
            0,           // Source offset
            0,           // Destination offset
            src.size   // Size
        );
        
        command_buffer.copyBuffer(src, dst, copy_region);

        end_one_time_command(command_buffer);
        
    }

    struct layout_transition_flags {
        vk::AccessFlags src_mask;
        vk::AccessFlags dst_mask;
        vk::PipelineStageFlags src_stage;
        vk::PipelineStageFlags dst_stage;
    };

    constexpr layout_transition_flags get_layout_transition_flags(const vk::ImageLayout &old_layout, const vk::ImageLayout &new_layout) {
        if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
            return layout_transition_flags {
                vk::AccessFlags(),
                vk::AccessFlagBits::eTransferWrite,
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eTransfer
            };
        } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            return layout_transition_flags {
                vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader
            };
        } else {
            throw std::invalid_argument("Unsupported layout transition");
        }
    }

    void Manager::layout_transition(vk::Image image, const vk::Format &format, const vk::ImageLayout &old_layout, const vk::ImageLayout &new_layout) {

        (void)format;

        auto layout_transition = get_layout_transition_flags(old_layout, new_layout);

        std::cout << "Layout transition" <<  (uint32_t)layout_transition.src_mask <<  "," << (uint32_t)layout_transition.dst_mask << "," << (uint32_t)layout_transition.src_stage << "," << (uint32_t)layout_transition.dst_stage << std::endl;

        auto command_buffer = begin_one_time_command();

        vk::ImageMemoryBarrier barrier(
            layout_transition.src_mask, // Src access mask
            layout_transition.dst_mask, // Dst access mask
            old_layout, // Old layout
            new_layout, // New layout
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eColor,
                0, // Base mip level
                1, // Level count
                0, // Base array layer
                1  // Layer count
            )
        );

        command_buffer.pipelineBarrier(
            layout_transition.src_stage,   // Src stage
            layout_transition.dst_stage,   // Dst stage
            vk::DependencyFlags(),      // Dependency flags
            nullptr,                    // Memory barrier
            nullptr,                    // Buffer barrier
            barrier                     // Image barrier
        );

        end_one_time_command(command_buffer);
    }

    void Manager::copy_buffer(BufferHandle &src_handle, vk::Image &dst_image, uint32_t width, uint32_t height, const vk::Format &format, const vk::ImageLayout &old_layout) {
        

        layout_transition(dst_image, format, old_layout, vk::ImageLayout::eTransferDstOptimal);

        {// Copy operation
            auto command_buffer = begin_one_time_command();

            vk::BufferImageCopy copy_region(
                0,  // Buffer offset
                0,  // Buffer row length
                0,  // Buffer image height
                vk::ImageSubresourceLayers(
                    vk::ImageAspectFlagBits::eColor,
                    0,  // Mip level
                    0,  // Base array layer
                    1   // Layer count
                ),
                vk::Offset3D(),                 // Offset
                vk::Extent3D(width, height, 1)  // Extent
            );

            command_buffer.copyBufferToImage(get_buffer(src_handle), dst_image, vk::ImageLayout::eTransferDstOptimal, copy_region);

            end_one_time_command(command_buffer);
        }

        layout_transition(dst_image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        
    }

    void* Manager::mapMemory(const BufferHandle &handle, const vk::MemoryMapFlags flags) {
        BufferContainer con = get_buffer(handle);
        //std::cout << "Mapping " << handle << " to memory" << std::endl;
        return p_device->mapMemory(get_memory(handle), con.offset, con.size, flags);
    }

    void Manager::unmapMemory(const BufferHandle &handle) {
        //std::cout << "Unmapping memory" << std::endl;
        p_device->unmapMemory(get_memory(handle));
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