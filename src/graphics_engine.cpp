

#include "graphics_engine.hpp"

#include <iostream>

Graphics::Graphics() {
    try{
        check_support();
        auto app_info = generate_app_info();
        create_instance(app_info);
        pick_physical_device();
        auto queue_family = pick_queue_family(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
        create_logical_device(queue_family);
        create_surface(queue_family);


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

void Graphics::check_support() {
    if (!glfwInit()) {
        std::cerr << "GLFW not initialized." << std::endl;
        exit(-1);
    }
    if (!glfwVulkanSupported()) {
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
        char const** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
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

    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    assert(!physicalDevices.empty());

    physical_device = physicalDevices[0];
}
                
uint32_t Graphics::pick_queue_family(vk::QueueFlags flags) {

    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = this->physical_device.getQueueFamilyProperties();

    size_t queueFamilyIndex = std::distance(queueFamilyProperties.begin(),
                                                    std::find_if(queueFamilyProperties.begin(),
                                                                    queueFamilyProperties.end(),
                                                                    [flags](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & flags; }));

    assert(queueFamilyIndex < queueFamilyProperties.size());

    return static_cast<uint32_t>(queueFamilyIndex);
}

void Graphics::create_logical_device(uint32_t queueFamilyIndex) {
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority);
    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo);

    std::vector<char const*> device_level_extensions;
    device_level_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    deviceCreateInfo.enabledExtensionCount = device_level_extensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = &device_level_extensions[0];
    device = physical_device.createDevice(deviceCreateInfo);

    // Picking a queue

    queue = device.getQueue(queueFamilyIndex,0);
}

void Graphics::create_surface(uint32_t queueFamilyIndex) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, AppName, NULL, NULL);

    // Attach Vulkan Surface to GLFW window
    VkSurfaceKHR raw_surface(surface);
    VkResult result = glfwCreateWindowSurface(instance, window, NULL, &raw_surface);
    if (result != VK_SUCCESS) {
        std::cerr << "GLFW: Unable to create surface, error code " << static_cast<int>(result) << std::endl;
        exit(-1);
    }
    surface = (vk::SurfaceKHR)raw_surface;

    if (!physical_device.getSurfaceSupportKHR(queueFamilyIndex, surface)) {
        std::cerr << "Surface not supported in queue family." << std::endl;
        exit(-1);
    }
}

void Graphics::create_swapchain() {
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
    width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(width)));
    height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(height)));
    swapChainInfo.setImageExtent(vk::Extent2D(width, height));
    
    swapChainInfo.setMinImageCount((capabilities.maxImageCount > 0) ? std::min(capabilities.maxImageCount, capabilities.minImageCount + 1) : capabilities.minImageCount + 1);
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapChainInfo.surface = surface;

    swapChainInfo.preTransform = capabilities.currentTransform;
    swapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainInfo.clipped = VK_TRUE;
    swapchain = device.createSwapchainKHR(swapChainInfo);
    swapChainImages = device.getSwapchainImagesKHR(swapchain);
}                

                




Graphics::~Graphics() {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            vkDestroySurfaceKHR((VkInstance)instance, VkSurfaceKHR(surface), nullptr);
            device.destroy();
            instance.destroy();
            glfwDestroyWindow(window); 
            glfwTerminate();
        }

void Graphics::start() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        loop();
    }
}