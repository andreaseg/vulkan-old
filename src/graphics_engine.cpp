
#include "graphics_engine.hpp"

#include <iostream>
#include <chrono>

Graphics* Graphics::current_engine;

const size_t MAX_CONCURRENT_FRAMES = 2;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 
    2, 3, 0
};

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    vk::VertexInputBindingDescription desc = vk::VertexInputBindingDescription(
        0,                              // Binding
        sizeof(Vertex),                 // Stride
        vk::VertexInputRate::eVertex    // Input rate
    );

    return desc;
}

std::array<vk::VertexInputAttributeDescription, 2>  Vertex::getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 2> attribs;

    attribs[0] = vk::VertexInputAttributeDescription(
        0,                          // Shader location
        0,                          // Binding
        vk::Format::eR32G32Sfloat,  // Format
        offsetof(Vertex, pos)       // Offset
    );

    attribs[1] = vk::VertexInputAttributeDescription(
        1,                              // Shader location
        0,                              // Binding
        vk::Format::eR32G32B32Sfloat,   // Format
        offsetof(Vertex, color)         // Offset
    );

    return attribs;
}


Graphics::Graphics() {
    try{
        dimensions = vk::Extent2D(640, 480);
        check_support();
        create_instance();
        pick_physical_device();
        pick_queue_family();
        create_logical_device();
        create_surface();
        create_swapchain();
        create_image_views();
        create_render_pass();
        create_descriptor_set_layout();
        create_pipeline();
        create_framebuffers();
        create_command_pool();
        memoryManager = vk_mem::Manager(&physical_device, &device, &queue, &commandPool);
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_set();
        create_vertex_buffers();
        create_index_buffers();
        create_texture_buffers();
        create_command_buffers();
        create_sync_objects();

    }
    catch (vk::SystemError err) {
        std::cerr << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    }
    catch (...) {
        std::cerr << "unknown error\n";
        exit(-2);
    }
    
}

uint32_t Graphics::getWidth() {
    return dimensions.width;
}

uint32_t Graphics::getHeight() {
    return dimensions.height;
}

float Graphics::getAspectRatio() {
    return (float) dimensions.width / (float) dimensions.height;
}

void Graphics::setDimensions(uint32_t width, uint32_t height) {
    dimensions.width = width;
    dimensions.height = height;
    has_been_resized = true;
}

void Graphics::check_support() {
    if (!glfw::glfwInit()) {
        std::cerr << "GLFW not initialized." << std::endl;
        exit(-1);
    }
    if (!glfw::glfwVulkanSupported()) {
        std::cerr << "Vulkan not supported." << std::endl;
        exit(-1);
    }
}

void Graphics::create_instance() {
    this->instance = vk_help::create_glfw_instance(AppName, EngineName);
}

void Graphics::pick_physical_device() {

    assert(instance);

    physical_device = vk_help::pick_first_physical_device(instance);
}
                
void Graphics::pick_queue_family() {

    assert(physical_device);

    this->queue_family = vk_help::pick_queue_family(physical_device);
}

void Graphics::create_logical_device() {

    assert(physical_device);

    device = vk_help::create_device_khr(physical_device, queue_family);

    // Picking a queue

    queue = device.getQueue(this->queue_family,0);
}

void Graphics::create_surface() {

    assert(instance);

    auto res = vk_help::create_glfw_surface_khr(physical_device, instance, queue_family, dimensions, AppName);

    window = std::get<0>(res);
    surface = std::get<1>(res);
}

void Graphics::create_swapchain() {

    assert(physical_device);
    assert(device);
    assert(surface);

    auto res = vk_help::create_standard_swapchain(physical_device, device, surface, dimensions, window, queue_family);

    swapchain = std::get<0>(res);
    swapChainImageFormat = std::get<1>(res);

    swapChainImages = device.getSwapchainImagesKHR(swapchain);
}                

void Graphics::create_image_views() {

    assert(swapChainImages.size() > 0);

    swapChainImageViews = vk_help::create_swapchain_image_views(device, swapChainImages, swapChainImageFormat);
}

void Graphics::create_render_pass() {

    assert(device);

    vk::AttachmentDescription color_attachment(
        vk::AttachmentDescriptionFlags(),
        swapChainImageFormat,               // Format
        vk::SampleCountFlagBits::e1,        // Samples
        vk::AttachmentLoadOp::eClear,       // Load Op
        vk::AttachmentStoreOp::eStore,      // Store op
        vk::AttachmentLoadOp::eDontCare,    // Stencil load op
        vk::AttachmentStoreOp::eDontCare,   // Stencil store op
        vk::ImageLayout::eUndefined,        // Initial layout
        vk::ImageLayout::ePresentSrcKHR     // Final layout
    );

    vk::AttachmentReference color_attachment_ref(
        0,                                          // Attachment
        vk::ImageLayout::eColorAttachmentOptimal    // Layout
    );

    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,   // Pipeline bindpoint
        0,                                  // Input atachment count
        nullptr,                            // Input attachments
        1,                                  // Color attachment count
        &color_attachment_ref,              // Color attachments
        nullptr,                            // Resolve attachments
        nullptr,                            // Depth stencil attachments,
        0,                                  // Preserve attachments count
        nullptr                             // Preserve attachments
    );

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL,                                // Source subpass
        0,                                                  // Destination subpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Source stage mask
        vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Destination stage mask
        vk::AccessFlags(),                                  // Source access mask
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite // Destination access ask
    );

    vk::RenderPassCreateInfo create_info(
        vk::RenderPassCreateFlags(),
        1,                  // Attachment count
        &color_attachment,  // Attachments,
        1,                  // Subpass count
        &subpass,           // Subpasses
        1,                  // Dependency count
        &dependency         // Subpass dependencies
    );

    renderPass = device.createRenderPass(create_info);

    if (!renderPass) {
        throw std::runtime_error("Failed to create render passs");
    }

    
}

void Graphics::create_descriptor_set_layout() {
    vk::DescriptorSetLayoutBinding ubo_layout_binding(
        0,                                  // Binding
        vk::DescriptorType::eUniformBuffer, // Type
        1,                                  // Count
        vk::ShaderStageFlagBits::eVertex,   // State flags
        nullptr                             // Sampler
    );

    vk::DescriptorSetLayoutCreateInfo create_info(
        vk::DescriptorSetLayoutCreateFlags(),
        1,                  // Binding count
        &ubo_layout_binding // Bindings
    );

    descriptorSetLayout = device.createDescriptorSetLayout(create_info);
}

void Graphics::create_pipeline() {

    assert(device);
    assert(renderPass);

    vk::ShaderModule vertex_shader = vk_help::load_precompiled_shader(device, "shaders/simple.vert.spv");
    vk::ShaderModule fragment_shader = vk_help::load_precompiled_shader(device, "shaders/simple.frag.spv");

    
    vk::PipelineShaderStageCreateInfo vert_stage_info(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eVertex,   // Stage
        vertex_shader,                      // Module
        "main",                             // Entry point
        nullptr                             // Specialization
    );

    vk::PipelineShaderStageCreateInfo frag_stage_info(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eFragment, // Stage
        fragment_shader,                    // Module
        "main",                             // Entry point
        nullptr                             // Specialization
    );

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {vert_stage_info, frag_stage_info};

    vk::VertexInputBindingDescription binding_description = Vertex::getBindingDescription();
    std::array<vk::VertexInputAttributeDescription, 2> attrib_description = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info(
        vk::PipelineVertexInputStateCreateFlags(),
        1,                                      // Bind description count
        &binding_description,                   // Bind descriptions
        (uint32_t) attrib_description.size(),   // Attribute description count
        attrib_description.data()               // Attribute descriptions
    );

    vk::PipelineInputAssemblyStateCreateInfo input_assembly(
        vk::PipelineInputAssemblyStateCreateFlags(),    // Flags
        vk::PrimitiveTopology::eTriangleList,           // Topology
        VK_FALSE                                        // Primitive restart enable
    );

    vk::Viewport viewport(
        0.0f,                       // X
        0.0f,                       // Y
        (float) dimensions.width,   // Width
        (float) dimensions.height,  // Height
        0.0f,                       // MinDepth
        1.0f                        // MaxDepth
    );

    vk::Rect2D scissors(
        {0, 0},     // Offset
        dimensions  // Extent
    );

    vk::PipelineViewportStateCreateInfo viewport_state(
        vk::PipelineViewportStateCreateFlags(),
        1,          // Viewport count
        &viewport,  // Viewports
        1,          // Scissor count
        &scissors   // Scissors
    );

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        vk::PipelineRasterizationStateCreateFlags(), 
        VK_FALSE,                       // Depth clamp
        VK_FALSE,                       // Rasterizer discard enable
        vk::PolygonMode::eFill,         // Polygon mode
        vk::CullModeFlagBits::eNone,    // Cull mode (TODO change)
        vk::FrontFace::eCounterClockwise,// Front face
        VK_FALSE,                       // Depth bias
        0,                              // Depth bias constant factor
        VK_FALSE,                       // Depth bias clamp
        0,                              // Depth bias slope factor
        1.0f                            // Line width
    );

    vk::PipelineMultisampleStateCreateInfo multisampling(
        vk::PipelineMultisampleStateCreateFlags(),
        vk::SampleCountFlagBits::e1,    // Rasterization samples
        VK_FALSE,                       // Shading enable,
        0,                              // Min sample shading
        nullptr,                        // Sample mask
        VK_FALSE,                       // Apha to coverage
        VK_FALSE                        // Alpha to one
    );

    vk::PipelineColorBlendAttachmentState color_blend_attachment(
        VK_FALSE,               // Blend enable
        vk::BlendFactor::eZero, // Source blend factor
        vk::BlendFactor::eZero, // Distant blend factor
        vk::BlendOp::eAdd,      // Color blend op
        vk::BlendFactor::eZero, // Surce alpha blend alpha
        vk::BlendFactor::eZero, // Distant alpha blend alpha
        vk::BlendOp::eAdd,      // Alpha blend op
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA // Color write mask
    );

    vk::PipelineColorBlendStateCreateInfo color_blending(
        vk::PipelineColorBlendStateCreateFlags(),
        VK_FALSE,               // Logic op enable
        vk::LogicOp::eClear,    // Logic op
        1,                      // Attachment count
        &color_blend_attachment // Color blend attachments
    );

    std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eLineWidth};
    vk::PipelineDynamicStateCreateInfo dynamic_state(
        vk::PipelineDynamicStateCreateFlags(),
        dynamic_states.size(),  // Dynamic state count
        &dynamic_states[0]      // Dynamic states
    );

    vk::PipelineLayoutCreateInfo pipeline_layout_info(
        vk::PipelineLayoutCreateFlags(),
        1,          // Descriptor set count
        &descriptorSetLayout,    // Descriptor sets
        0,          // Push constant range counts
        nullptr     // Push constant ranges
    );

    this->pipelineLayout = device.createPipelineLayout(pipeline_layout_info);

    if (!pipelineLayout) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    vk::GraphicsPipelineCreateInfo pipeline_create_info(
        vk::PipelineCreateFlags(),
        shader_stages.size(),   // Stage count
        &shader_stages[0],      // Stages
        &vertex_input_info,     // Vertex
        &input_assembly,        // Input
        nullptr,                // Tesselation
        &viewport_state,        // Viewport
        &rasterizer,            // Rasterization
        &multisampling,         // Multisampling
        nullptr,                // Depth stencil
        &color_blending,        // Color blend
        nullptr,                // Dynamic state
        pipelineLayout,         // Layout
        renderPass,             // Render pass
        0                       // Subpass
    );

    graphicsPipeline = device.createGraphicsPipeline(nullptr, pipeline_create_info);

    if (!graphicsPipeline) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    device.destroyShaderModule(fragment_shader);
    device.destroyShaderModule(vertex_shader);

}

void Graphics::create_framebuffers() {
    assert(swapChainImageViews.size() != 0);
    swapChainFrameBuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::vector<vk::ImageView> attachments = {swapChainImageViews[i]};

        vk::FramebufferCreateInfo create_info(
            vk::FramebufferCreateFlags(),
            renderPass,                 // Render pass
            attachments.size(),         // Attachment count
            &attachments[0],            // Attachments
            this->dimensions.width,     // Width
            this->dimensions.height,    // Height
            1                           // Layers
        );

        swapChainFrameBuffers[i] = device.createFramebuffer(create_info);

        assert(swapChainFrameBuffers[i]);
    }
}

void Graphics::create_command_pool() {
    vk::CommandPoolCreateInfo create_info(
        vk::CommandPoolCreateFlags(),
        queue_family                // Queue to use
    );

    commandPool = device.createCommandPool(create_info);

    assert(commandPool);
}

void Graphics::create_vertex_buffers() {

    vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    vk_mem::BufferHandle staging_buffer = memoryManager.create_transfer_buffer(buffer_size);
    vertexBuffer = memoryManager.create_vertex_buffer(buffer_size);

    void* data = memoryManager.mapMemory(staging_buffer);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    memoryManager.unmapMemory(staging_buffer);

    memoryManager.copy_buffer(staging_buffer, vertexBuffer);

    memoryManager.free(staging_buffer);
    
}

void Graphics::create_index_buffers() {

    vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();

    vk_mem::BufferHandle staging_buffer = memoryManager.create_transfer_buffer(buffer_size);
    indexBuffer = memoryManager.create_index_buffer(buffer_size);

    void* data = memoryManager.mapMemory(staging_buffer);
    memcpy(data, indices.data(), (size_t) buffer_size);
    memoryManager.unmapMemory(staging_buffer);

    memoryManager.copy_buffer(staging_buffer, indexBuffer);

    memoryManager.free(staging_buffer);
}

void Graphics::create_uniform_buffers() {

    vk::DeviceSize buffer_size = sizeof(Transformations);

    uniformBuffers.resize(swapChainImages.size());

    for (auto &buffer : uniformBuffers) {
        buffer = memoryManager.create_uniform_buffer(buffer_size);
    }
}

void Graphics::create_texture_buffers() {
    std::cout << "Creating texture buffers" << std::endl;
    auto image = vk_help::load_image("textures/texture.jpg");

    vk::DeviceSize buffer_size = image.width * image.height * 4;

    vk_mem::BufferHandle staging_buffer = memoryManager.create_transfer_buffer(buffer_size);
    
    {
        vk::ImageCreateInfo create_info(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D,                         // Type
            vk::Format::eR8G8B8A8Unorm,                 // Format
            vk::Extent3D(image.width, image.height, 1), // Extent
            1,                                          // Mip levels
            1,                                          // Array layers
            vk::SampleCountFlagBits::e1,                // Samples
            vk::ImageTiling::eOptimal,                  // Tiling
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, // Usage
            vk::SharingMode::eExclusive,                // Sharing mode
            1,                                          // Queue family count
            &queue_family,                              // Queue families
            vk::ImageLayout::eUndefined                 // Layout
        );

        textureImage = device.createImage(create_info);
    }

    {
        auto mem_reqs = device.getImageMemoryRequirements(textureImage);
        auto mem_type = memoryManager.find_memory_type(mem_reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
        std::cout << "Allocating memory of type " << mem_type << " for image." << std::endl;
        vk::MemoryAllocateInfo alloc_info(mem_reqs.size, mem_type);

        textureImageMemory = device.allocateMemory(alloc_info);

        device.bindImageMemory(textureImage, textureImageMemory, 0 /* memory offset */);
    }

    void* data = memoryManager.mapMemory(staging_buffer);
    memcpy(data, image.image, (size_t) buffer_size);
    memoryManager.unmapMemory(staging_buffer);

    memoryManager.copy_buffer(staging_buffer, textureImage, image.width, image.height, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined);

    memoryManager.free(staging_buffer);
    std::cout << "Finished wih texture" << std::endl;
}

void Graphics::create_descriptor_pool() {
    vk::DescriptorPoolSize pool_size(
        vk::DescriptorType::eUniformBuffer, // Type
        (uint32_t) swapChainImages.size()   // Count
    );

    vk::DescriptorPoolCreateInfo create_info(
        vk::DescriptorPoolCreateFlags(),
        (uint32_t) swapChainImages.size(),  // Max sets
        1,                                  // Pool count
        &pool_size                          // Pool sizes
    );

    descriptorPool = device.createDescriptorPool(create_info);
}

void Graphics::create_descriptor_set() {
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

    vk::DescriptorSetAllocateInfo alloc_info(
        descriptorPool,             // Descriptor pool
        (uint32_t) swapChainImages.size(),  // Descriptor count
        layouts.data()              // Descriptor layouts
    );
    
    descriptorSets.resize(swapChainImages.size());
    descriptorSets = device.allocateDescriptorSets(alloc_info);


    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk::DescriptorBufferInfo buffer_info(
            memoryManager.get_buffer(uniformBuffers[i]),// Buffer
            0,                                          // Offset
            sizeof(Transformations)                     // Range
        );

        vk::WriteDescriptorSet write_desc(
            descriptorSets[i],                  // Dst set
            0,                                  // Dst binding
            0,                                  // Dst array element
            1,                                  // Description count
            vk::DescriptorType::eUniformBuffer, // Description type
            nullptr,                            // Image info
            &buffer_info,                       // Buffer info
            nullptr                             // Texel buffer view
        );

        device.updateDescriptorSets(1, &write_desc, 0, nullptr);
    }
}

void Graphics::create_command_buffers() {
    assert(swapChainFrameBuffers.size() != 0);
    commandBuffers.resize(swapChainFrameBuffers.size());

    vk::CommandBufferAllocateInfo alloc_info(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        (uint32_t) commandBuffers.size()
    );

    commandBuffers = device.allocateCommandBuffers(alloc_info);

    vk::ClearValue clear_color;
    clear_color.color.setFloat32({0.0f, 0.0f, 0.2f, 1.0f});
    std::vector<vk::ClearValue> clear_values = {clear_color};

    vk::Rect2D render_area(
        {0,0},              // Offset
        this->dimensions    // Extent
    );


    for (size_t i = 0; i < commandBuffers.size(); i++) {

        vk::CommandBuffer *cmd = &commandBuffers[i];

        vk::CommandBufferBeginInfo begin_info(
            vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            nullptr // Inheritance
        );

        cmd->begin(begin_info);

        vk::RenderPassBeginInfo render_pass_info(
            renderPass,                 // Render pass
            swapChainFrameBuffers[i],   // Framebuffer
            render_area,                // Render area
            clear_values.size(),        // Clear value count
            &clear_values[0]            // Clear values
        );

        cmd->beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

        cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        auto vertex_buffer_container = memoryManager.get_buffer(vertexBuffer);
        vk::Buffer vertex_buffers[] = {vertex_buffer_container};
        vk::DeviceSize vertex_buffer_offsets[] = {0};
        auto index_buffer = memoryManager.get_buffer(indexBuffer);


        cmd->bindVertexBuffers(
            0,                      // First binding
            1,                      // Buffer count
            vertex_buffers,         // Internal buffer offsets
            vertex_buffer_offsets   // Offsets
        );

        cmd->bindIndexBuffer(
            index_buffer,           // Buffer
            0,                      // Internal buffer offset
            vk::IndexType::eUint16  // Index type
        );

        
        cmd->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,   // Pipeline bind point
            pipelineLayout,                     // Pipeline layout
            0,                                  // First set
            1,                                  // Set count
            &descriptorSets[i],                 // Descriptor sets
            0,                                  // Dynamic offset count
            nullptr                             // Dynamic offsets
        );
        
        

        cmd->drawIndexed(
            (uint32_t) indices.size(), // Index count
            1, // Instance count
            0, // First index,
            0, // Vertex offset
            0  // First instance
        );

        cmd->endRenderPass();

        cmd->end();
    }


}

void Graphics::create_sync_objects() {
    frameSyncObjects.resize(MAX_CONCURRENT_FRAMES);

    vk::FenceCreateInfo fence_create_info(
        vk::FenceCreateFlagBits::eSignaled
    );
    
    for (auto &f : frameSyncObjects) {
        f.imageAvailableSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
        f.renderFinishedSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
        f.inFlightFence = device.createFence(fence_create_info);
    }
}

const auto start_time = std::chrono::steady_clock::now();

void Graphics::update_uniform_buffers(uint32_t image_index) {
    auto current_time = std::chrono::steady_clock::now();

    float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;

    auto up = glm::vec3(0.0f, 0.0f, 1.0f);
    float fov = 45.0f;

    Transformations t;
    t.model = glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(90.0f),
        up
    );
    t.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        up
    );
    t.proj = glm::perspective(
        glm::radians(fov),
        getAspectRatio(),
        0.1f,
        100.0f
    );
    t.proj[1][1] *= -1; // Y-coordinate fix

    void* data = memoryManager.mapMemory(uniformBuffers[image_index]);
    memcpy(data, &t, sizeof(t));
    memoryManager.unmapMemory(uniformBuffers[image_index]);

}


void Graphics::draw_frame() {
    const uint64_t timeout = std::numeric_limits<uint64_t>::max();

    device.waitForFences(
        1,                              // Fence count
        &frameSyncObjects[current_frame].inFlightFence, // Fences
        VK_TRUE,                        // Wait for all
        timeout                         // Timeout
    );

    device.resetFences(
        1,                              // Fence count
        &frameSyncObjects[current_frame].inFlightFence  // Fences
    );

    if (has_been_resized) {
        recreate_swapchain();
    }

    uint32_t image_index;
    vk::Result result = device.acquireNextImageKHR(swapchain, timeout, frameSyncObjects[current_frame].imageAvailableSemaphore, nullptr, &image_index);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreate_swapchain();
        return;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire image");
    }

    update_uniform_buffers(image_index);

    std::vector<vk::Semaphore> wait_semaphores = {frameSyncObjects[current_frame].imageAvailableSemaphore};
    std::vector<vk::Semaphore> signal_semaphores = {frameSyncObjects[current_frame].renderFinishedSemaphore};
    std::vector<vk::PipelineStageFlags> wait_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submit_info(
        wait_semaphores.size(),         // Wait semaphores count
        &wait_semaphores[0],            // Wait semaphores
        wait_stages.data(),             // Wait stages
        1,                              // Command buffer count
        &commandBuffers[image_index],   // Command buffers
        signal_semaphores.size(),       // Signal semaphores count
        &signal_semaphores[0]           // Signal semaphores
    );

    queue.submit(submit_info, frameSyncObjects[current_frame].inFlightFence);

    vk::PresentInfoKHR present_info(
        signal_semaphores.size(), // Wait semaphore count
        &signal_semaphores[0],    // Wait semaphores
        1,                      // Swapchain count
        &swapchain,             // Swapchains
        &image_index,           // Image index
        nullptr                 // Result array
    );

    result = queue.presentKHR(present_info);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        recreate_swapchain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present image");
    }

    current_frame = (current_frame + 1) % MAX_CONCURRENT_FRAMES;
    
}

void Graphics::recreate_swapchain() {
    device.waitIdle();

    clean_up_swapchain();

    create_swapchain();
    create_image_views();
    create_render_pass();
    create_pipeline();
    create_framebuffers();
    create_command_buffers();

    this->has_been_resized = false;
}

void Graphics::clean_up_swapchain() {
    for (auto &framebuffer : swapChainFrameBuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    
    device.freeCommandBuffers(commandPool, commandBuffers);

    device.destroyPipeline(graphicsPipeline);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);

    for (auto &image_view: swapChainImageViews) {
        device.destroyImageView(image_view);
    }

    device.destroySwapchainKHR(swapchain);
}

Graphics::~Graphics() {
    clean_up_swapchain();

    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorSetLayout(descriptorSetLayout);

    device.destroyImage(textureImage);
    device.freeMemory(textureImageMemory);


    memoryManager.destroy();
    for (auto &sync_objects : frameSyncObjects) {
        device.destroyFence(sync_objects.inFlightFence);
        device.destroySemaphore(sync_objects.imageAvailableSemaphore);
        device.destroySemaphore(sync_objects.renderFinishedSemaphore);
    }
    device.destroyCommandPool(commandPool);
    instance.destroySurfaceKHR(surface);

    device.destroy();
    instance.destroy();
    glfw::glfwDestroyWindow(window); 
    glfw::glfwTerminate();
}

void Graphics::start() {
    current_engine = this;
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_callback);
    glfwSetWindowFocusCallback(window, glfw_window_focus_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_pos_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    std::cout << "Starting" << std::endl;

    while (!glfw::glfwWindowShouldClose(window)) {
        glfw::glfwPollEvents();
        loop();
        draw_frame();
    }

    device.waitIdle();
}

void Graphics::close() {
    glfwSetWindowShouldClose(window, (int)true);
}

void Graphics::glfw_key_callback(glfw::GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;

    #ifdef DEBUG
    if (current_engine == nullptr) {
        std::cerr << "Key-event (" << key << ") recored before initialization" << std::endl;
        return;
    }
    #endif

    auto range = current_engine->key_callbacks.equal_range(key);
    for_each(
        range.first,
        range.second,
        [action, mods](std::unordered_multimap<int, key_event>::value_type &x){
            key_event* e = &x.second;
            if(e->action == action && e->modifier == mods) {
                e->callback();
                }
            }
    );
}

void Graphics::glfw_mouse_callback(glfw::GLFWwindow *window, int key, int action, int mods) {

    #ifdef DEBUG
    if (current_engine == nullptr) {
        std::cerr << "Mouse-event (" << key << ") recored before initialization" << std::endl;
        return;
    }
    #endif

    double xpos, ypos;
    glfw::glfwGetCursorPos(window, &xpos, &ypos);

    auto range = current_engine->mouse_callbacks.equal_range(key);
    for_each(
        range.first,
        range.second,
        [action, mods, xpos, ypos](std::unordered_multimap<int, mouse_event>::value_type &x){
            mouse_event* e = &x.second;
            if(e->action == action && e->modifier == mods) {
                e->callback(xpos, ypos);
                }
            }
    );
}

void Graphics::glfw_window_focus_callback(glfw::GLFWwindow *window, int status) {
    (void)window;

    #ifdef DEBUG
    if (current_engine == nullptr) {
        std::cerr << "Focus-event (" << status << ") recored before initialization" << std::endl;
        return;
    }
    #endif

    current_engine->has_focus = (bool)status;

    for (auto &call : current_engine->window_focus_callbacks) {
        if (current_engine->has_focus == call.focus) {
            call.callback();
        }
    }
}

void Graphics::glfw_mouse_pos_callback(glfw::GLFWwindow *window, double xpos, double ypos) {
    (void)window;

    #ifdef DEBUG
    if (current_engine == nullptr) {
        std::cerr << "Mouse-pos-event recored before initialization" << std::endl;
        return;
    }
    #endif

    for (auto &call : current_engine->mouse_pos_callbacks) {
        call(xpos, ypos);
    }
}

void Graphics::window_size_callback(glfw::GLFWwindow *window, int width, int height) {
    (void)window;
    current_engine->setDimensions(width, height);
}

void Graphics::addKeyCallback(int key, int action, int modifier, std::function<void()> callback) {
    key_callbacks.emplace(key, key_event{action, modifier, callback});
}

void Graphics::addMouseCallback(int button, int action, int modifier, std::function<void(double xpos, double ypos)> callback) {
    mouse_callbacks.emplace(button, mouse_event{action, modifier, callback});
}

void Graphics::addMousePosCallback(std::function<void(double xpos, double ypos)> callback) {
    mouse_pos_callbacks.push_back(callback);
}

void Graphics::addWindowFocusCallback(bool focus, std::function<void()> callback) {
    window_focus_callbacks.push_back(focus_event{focus, callback});
}