

#include "graphics_engine.hpp"

#include <iostream>
#include <fstream>

Graphics* Graphics::current_engine;

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

    vk::InstanceCreateInfo createInstanceInfo = vk::InstanceCreateInfo({}, &app_info);
    createInstanceInfo.enabledExtensionCount = extensions.size();
    createInstanceInfo.ppEnabledExtensionNames = &extensions[0];

    // Creating an instance
    this->instance = vk::createInstance(createInstanceInfo);
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

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queue_family, 1, &queuePriority);
    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo);

    std::vector<char const*> device_level_extensions;
    device_level_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    deviceCreateInfo.enabledExtensionCount = device_level_extensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = &device_level_extensions[0];
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

    vk::SwapchainCreateInfoKHR swapChainInfo;

    // Selecting format for swapchain
    auto surface_formats = physical_device.getSurfaceFormatsKHR(surface);
    if (surface_formats.size() == 1 && surface_formats[0].format == vk::Format::eUndefined) {
        swapChainInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapChainInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    } else if (std::any_of(surface_formats.begin(), surface_formats.end(), 
        [](vk::SurfaceFormatKHR const& f) {return f.format == vk::Format::eUndefined && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        })) {
        swapChainInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapChainInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    } else {
        swapChainInfo.setImageFormat(surface_formats[0].format);
        swapChainInfo.setImageColorSpace(surface_formats[0].colorSpace);
    }

    swapChainImageFormat = swapChainInfo.imageFormat;

    // Selecting presentation mode
    auto presentation_modes = physical_device.getSurfacePresentModesKHR(surface);
    if (std::any_of(presentation_modes.begin(), presentation_modes.end(), [](vk::PresentModeKHR const& p){return p == vk::PresentModeKHR::eMailbox;})) {
        swapChainInfo.presentMode = vk::PresentModeKHR::eMailbox;
    } else if (std::any_of(presentation_modes.begin(), presentation_modes.end(), [](vk::PresentModeKHR const& p){return p == vk::PresentModeKHR::eImmediate;})) {
        swapChainInfo.presentMode = vk::PresentModeKHR::eImmediate;
    } else {
        swapChainInfo.presentMode = vk::PresentModeKHR::eFifo;
    }

    // Selecting dimensions
    auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
    this->dimensions.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, this->dimensions.width));
    this->dimensions.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, this->dimensions.height));
    swapChainInfo.setImageExtent(this->dimensions);
    
    swapChainInfo.setMinImageCount((capabilities.maxImageCount > 0) ? std::min(capabilities.maxImageCount, capabilities.minImageCount + 1) : capabilities.minImageCount + 1);
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapChainInfo.surface = surface;

    swapChainInfo.preTransform = capabilities.currentTransform;
    swapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainInfo.clipped = VK_TRUE;

    swapchain = device.createSwapchainKHR(swapChainInfo);

    if (!swapchain) {
        throw std::runtime_error("Failed to create swapchain");
    }

    swapChainImages = device.getSwapchainImagesKHR(swapchain);
}                

void Graphics::create_image_views() {

    assert(swapchain);

    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk::ImageViewCreateInfo create_info;
        create_info.image = swapChainImages[i];
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = swapChainImageFormat;

        create_info.components.r = vk::ComponentSwizzle::eIdentity;
        create_info.components.g = vk::ComponentSwizzle::eIdentity;
        create_info.components.b = vk::ComponentSwizzle::eIdentity;
        create_info.components.a = vk::ComponentSwizzle::eIdentity;

        create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

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

    vk::ShaderModuleCreateInfo create_info;
    create_info.codeSize = size;
    create_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    vk::ShaderModule shader = device.createShaderModule(create_info);

    if(!shader) {
        throw std::runtime_error("Failed to create shader");
    }

    return shader;
}

void Graphics::create_render_pass() {

    assert(device);

    vk::AttachmentDescription color_attachment;
    color_attachment.format = swapChainImageFormat;
    color_attachment.samples = vk::SampleCountFlagBits::e1;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.initialLayout = vk::ImageLayout::eUndefined;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_attachment_ref;
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    vk::RenderPassCreateInfo create_info;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &color_attachment;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;

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

    
    vk::PipelineShaderStageCreateInfo vert_stage_info;
    vert_stage_info.stage = vk::ShaderStageFlagBits::eVertex;
    vert_stage_info.module = vertex_shader;
    vert_stage_info.pName = "main";

    vk::PipelineShaderStageCreateInfo frag_stage_info;
    frag_stage_info.stage = vk::ShaderStageFlagBits::eFragment;
    frag_stage_info.module = fragment_shader;
    frag_stage_info.pName = "main";

    vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewport_state;
    {
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
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissors;
    }

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasClamp = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo color_blending;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    vk::DynamicState dynamic_states[] = {vk::DynamicState::eViewport, vk::DynamicState::eLineWidth};
    vk::PipelineDynamicStateCreateInfo dynamic_state;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    vk::PipelineLayoutCreateInfo pipeline_layout_info;

    this->pipelineLayout = device.createPipelineLayout(pipeline_layout_info);

    if (!pipelineLayout) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    vk::GraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr; // Optional
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = nullptr; // Optional
    pipeline_info.layout = pipelineLayout;
    pipeline_info.renderPass = renderPass;
    pipeline_info.subpass = 0;

    graphicsPipeline = device.createGraphicsPipeline(nullptr, pipeline_info);

    if (!graphicsPipeline) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device, fragment_shader, nullptr);
    vkDestroyShaderModule(device, vertex_shader, nullptr);
}




Graphics::~Graphics() {
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
    }
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