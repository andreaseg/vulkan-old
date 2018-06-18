

#include "graphics_engine.hpp"

#include <iostream>
#include <fstream>
#include <ctime>

Graphics* Graphics::current_engine;

const size_t MAX_CONCURRENT_FRAMES = 2;

Graphics::Graphics() {
    try{
        dimensions = vk::Extent2D(640, 480);
        check_support();
        auto app_info = generate_app_info();
        create_instance(app_info);
        pick_physical_device();
        pick_queue_family(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
        create_logical_device();
        create_surface();
        create_swapchain();
        create_image_views();
        create_render_pass();
        create_pipeline();
        create_framebuffers();
        create_command_pool();
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

void Graphics::setDimensions(uint32_t width, uint32_t height) {
    dimensions.width = width;
    dimensions.height = height;
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

vk::ApplicationInfo Graphics::generate_app_info() {
    return vk::ApplicationInfo(AppName, 1, EngineName, 1, VK_API_VERSION_1_1);
}

void Graphics::create_instance(vk::ApplicationInfo app_info) {

    // Extensions needed
    std::vector<char const*> extensions;
    {
        uint32_t extensionCount = 0;
        char const** glfw_extensions = glfw::glfwGetRequiredInstanceExtensions(&extensionCount);
        for (uint32_t i = 0; i < extensionCount; i++) {
            extensions.push_back(&glfw_extensions[i][0]);
        }
    }
    #ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    #endif
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    vk::InstanceCreateInfo create_instance_info(
        vk::InstanceCreateFlags(),
        &app_info,                  // Application Info
        0,                          // Layer count
        nullptr,                    // Layers
        extensions.size(),          // Extension count
        &extensions[0]              // Extensions
        );

    // Creating an instance
    this->instance = vk::createInstance(create_instance_info);
}

void Graphics::pick_physical_device() {

    assert(instance);

    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    assert(!physicalDevices.empty());

    physical_device = physicalDevices[0];
}
                
void Graphics::pick_queue_family(vk::QueueFlags flags) {

    assert(physical_device);

    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physical_device.getQueueFamilyProperties();

    size_t queueFamilyIndex = std::distance(queueFamilyProperties.begin(),
                                                    std::find_if(queueFamilyProperties.begin(),
                                                                    queueFamilyProperties.end(),
                                                                    [flags](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & flags; }));

    assert(queueFamilyIndex < queueFamilyProperties.size());

    this->queue_family = static_cast<uint32_t>(queueFamilyIndex);
}

void Graphics::create_logical_device() {

    assert(physical_device);

    std::vector<char const*> device_level_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        queue_family,                   // Queue Family
        1,                              // Queue count
        &queuePriority                  // Queue priority
        );
    vk::DeviceCreateInfo deviceCreateInfo(
        vk::DeviceCreateFlags(),
        1,                              // Queue create info count
        &deviceQueueCreateInfo,         // Queue create info
        0,                              // Enabled layer count
        nullptr,                        // Enabled layers
        device_level_extensions.size(), // Enabled extensions count
        &device_level_extensions[0]     // Enabled extensions
        );

    device = physical_device.createDevice(deviceCreateInfo);

    // Picking a queue

    queue = device.getQueue(this->queue_family,0);
}

void Graphics::create_surface() {

    assert(instance);

    if (dimensions.width == 0 || dimensions.height == 0) {
        throw std::runtime_error("Error: Window dimensions not set");
    }

    glfw::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfw::glfwCreateWindow(dimensions.width, dimensions.height, AppName, NULL, NULL);

    // Attach Vulkan Surface to GLFW window
    VkSurfaceKHR raw_surface(surface);
    VkResult result = glfw::glfwCreateWindowSurface(instance, window, NULL, &raw_surface);
    if (result != VK_SUCCESS) {
        std::cerr << "GLFW: Unable to create surface, error code " << static_cast<int>(result) << std::endl;
        exit(-1);
    }
    surface = (vk::SurfaceKHR)raw_surface;

    if (!physical_device.getSurfaceSupportKHR(this->queue_family, surface)) {
        std::cerr << "Surface not supported in queue family." << std::endl;
        exit(-1);
    }
}

void Graphics::create_swapchain() {

    assert(physical_device);
    assert(device);
    assert(surface);

    vk::Format image_format;
    vk::ColorSpaceKHR color_space;

    // Selecting format for swapchain
    auto surface_formats = physical_device.getSurfaceFormatsKHR(surface);
    if (surface_formats.size() == 1 && surface_formats[0].format == vk::Format::eUndefined) {
        image_format = vk::Format::eB8G8R8A8Unorm;
        color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
    } else if (std::any_of(surface_formats.begin(), surface_formats.end(), 
        [](vk::SurfaceFormatKHR const& f) {return f.format == vk::Format::eUndefined && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        })) {
        image_format = vk::Format::eB8G8R8A8Unorm;
        color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
    } else {
        image_format = surface_formats[0].format;
        color_space = surface_formats[0].colorSpace;
    }

    vk::PresentModeKHR present_mode;

    // Selecting presentation mode
    auto presentation_modes = physical_device.getSurfacePresentModesKHR(surface);
    if (std::any_of(presentation_modes.begin(), presentation_modes.end(), [](vk::PresentModeKHR const& p){return p == vk::PresentModeKHR::eMailbox;})) {
        present_mode = vk::PresentModeKHR::eMailbox;
    } else if (std::any_of(presentation_modes.begin(), presentation_modes.end(), [](vk::PresentModeKHR const& p){return p == vk::PresentModeKHR::eImmediate;})) {
        present_mode = vk::PresentModeKHR::eImmediate;
    } else {
        present_mode = vk::PresentModeKHR::eFifo;
    }

    // Selecting dimensions
    auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
    this->dimensions.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, this->dimensions.width));
    this->dimensions.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, this->dimensions.height));
    
    uint32_t min_image_count = (capabilities.maxImageCount > 0) ? std::min(capabilities.maxImageCount, capabilities.minImageCount + 1) : capabilities.minImageCount + 1;

    vk::SwapchainCreateInfoKHR swap_chain_info(
        vk::SwapchainCreateFlagsKHR(),
        surface,                                    // SurfaceKHR
        min_image_count,                            // Min image count
        image_format,                               // Image format
        color_space,                                // Color space
        this->dimensions,                           // Extent
        1,                                          // Image array layers
        vk::ImageUsageFlagBits::eColorAttachment,   // Image usage flags
        vk::SharingMode::eExclusive,                // Image sharing mode
        1,                                          // Queue family index count
        &queue_family,                              // Queue family indices
        capabilities.currentTransform,              // Surface transform flags
        vk::CompositeAlphaFlagBitsKHR::eOpaque,     // Composite alpha flags
        present_mode,                               // Present mode
        VK_TRUE,                                    // Clipped
        nullptr                                     // Old swapchain
    );

    swapchain = device.createSwapchainKHR(swap_chain_info);
    swapChainImageFormat = image_format;

    if (!swapchain) {
        throw std::runtime_error("Failed to create swapchain");
    }

    swapChainImages = device.getSwapchainImagesKHR(swapchain);
}                

void Graphics::create_image_views() {

    assert(swapchain);

    swapChainImageViews.resize(swapChainImages.size());


    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk::ImageViewCreateInfo create_info(
            vk::ImageViewCreateFlags(),
            swapChainImages[i],                     // Image
            vk::ImageViewType::e2D,                 // View type
            swapChainImageFormat,                   // Format
            vk::ComponentMapping(                   // Components
                vk::ComponentSwizzle::eIdentity,    // R
                vk::ComponentSwizzle::eIdentity,    // G
                vk::ComponentSwizzle::eIdentity,    // B
                vk::ComponentSwizzle::eIdentity     // A
            ),                 
            vk::ImageSubresourceRange(              // Subresource range
                vk::ImageAspectFlagBits::eColor,    // Aspect mask
                0,                                  // Base mip level
                1,                                  // Level count
                0,                                  // Base array layer
                1                                   // Layer count
            )          
        );

        swapChainImageViews[i] = device.createImageView(create_info);
    }
}

vk::ShaderModule Graphics::load_precompiled_shader(const std::string &filename) {

    assert(device);

    // Load binary shader data
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open()) {
        throw std::runtime_error("Could not open shader");
    }

    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);

    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();

    vk::ShaderModuleCreateInfo create_info(
        vk::ShaderModuleCreateFlags(),
        size, // Code size
        reinterpret_cast<const uint32_t*>(buffer.data()) // Code
    );

    vk::ShaderModule shader = device.createShaderModule(create_info);

    if(!shader) {
        throw std::runtime_error("Failed to create shader");
    }

    return shader;
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

void Graphics::create_pipeline() {

    assert(device);
    assert(renderPass);

    vk::ShaderModule vertex_shader = load_precompiled_shader("shaders/simple.vert.spv");
    vk::ShaderModule fragment_shader = load_precompiled_shader("shaders/simple.frag.spv");

    
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

    vk::PipelineVertexInputStateCreateInfo vertex_input_info(
        vk::PipelineVertexInputStateCreateFlags(),
        0,          // Bind description count
        nullptr,    // Bind descriptions
        0,          // Attribute description count
        nullptr     // Attribute descriptions
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
        VK_FALSE,                       // Depth clamp,
        VK_FALSE,                       // Rasterizer discard enable,
        vk::PolygonMode::eFill,         // Polygon mode
        vk::CullModeFlagBits::eBack,    // Cull mode,
        vk::FrontFace::eClockwise,      // Front face,
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
        0,          // Descriptor set count
        nullptr,    // Descriptor sets
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

    vkDestroyShaderModule(device, fragment_shader, nullptr);
    vkDestroyShaderModule(device, vertex_shader, nullptr);

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
    clear_color.color.setFloat32({0.0f, 0.0f, 0.0f, 1.0f});
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

        cmd->draw(
            3, // Vertex count
            1, // Instance count
            0, // First vertex
            0  // First instance
        );

        cmd->endRenderPass();

        cmd->end();
    }
}

void Graphics::create_sync_objects() {
    imageAvailableSemaphores.resize(MAX_CONCURRENT_FRAMES);
    renderFinishedSemaphores.resize(MAX_CONCURRENT_FRAMES);
    inFlightFences.resize(MAX_CONCURRENT_FRAMES);

    vk::FenceCreateInfo fence_create_info(
        vk::FenceCreateFlagBits::eSignaled
    );
    
    for (size_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
        imageAvailableSemaphores[i] = device.createSemaphore(vk::SemaphoreCreateInfo());
        renderFinishedSemaphores[i] = device.createSemaphore(vk::SemaphoreCreateInfo());
        inFlightFences[i] = device.createFence(fence_create_info);
    }
}

uint64_t framecounter = 0;
std::clock_t old_time;

void Graphics::draw_frame() {
    const uint64_t timeout = std::numeric_limits<uint64_t>::max();

    device.waitForFences(
        1,                              // Fence count
        &inFlightFences[current_frame], // Fences
        VK_TRUE,                        // Wait for all
        timeout                         // Timeout
    );

    device.resetFences(
        1,                              // Fence count
        &inFlightFences[current_frame]  // Fences
    );

    uint32_t image_index;
    device.acquireNextImageKHR(swapchain, timeout, imageAvailableSemaphores[current_frame], nullptr, &image_index);

    std::vector<vk::Semaphore> wait_semaphores = {imageAvailableSemaphores[current_frame]};
    std::vector<vk::Semaphore> signal_semaphores = {renderFinishedSemaphores[current_frame]};
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

    queue.submit(submit_info, inFlightFences[current_frame]);

    vk::PresentInfoKHR present_info(
        signal_semaphores.size(), // Wait semaphore count
        &signal_semaphores[0],    // Wait semaphores
        1,                      // Swapchain count
        &swapchain,             // Swapchains
        &image_index,           // Image index
        nullptr                 // Result array
    );

    queue.presentKHR(present_info);

    current_frame = (current_frame + 1) % MAX_CONCURRENT_FRAMES;

    framecounter++;
    if ((framecounter + 1) % 1000 == 0) {
        auto new_time = std::clock();
        auto elapsed_time = new_time - old_time;
        old_time = new_time;
        std::cout << "Frames per second " << (1000.0 * 1000.0 / (float)elapsed_time) << std::endl;
    }
    
}

Graphics::~Graphics() {
    for (auto &fence : inFlightFences) {
        vkDestroyFence(device, fence, nullptr);
    }
    for (auto &semaphore : imageAvailableSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (auto &semaphore : renderFinishedSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto &framebuffer : swapChainFrameBuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto &image_view: swapChainImageViews) {
        vkDestroyImageView(device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR((VkInstance)instance, VkSurfaceKHR(surface), nullptr);
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