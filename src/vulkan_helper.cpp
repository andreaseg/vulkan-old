#include "vulkan_helper.hpp"
#include <iostream>
#include <fstream>

namespace vk_help {
    vk::Instance create_glfw_instance(const std::string &app_name, const std::string &engine_name, const std::vector<char const*> *optional_extensions) {
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

        if (optional_extensions != nullptr) {
            for (auto &ext : *optional_extensions) {
                extensions.push_back(ext);
            }
        }

        vk::ApplicationInfo app_info(app_name.c_str(), 1, engine_name.c_str(), 1, VK_API_VERSION_1_1);

        vk::InstanceCreateInfo create_instance_info(
            vk::InstanceCreateFlags(),
            &app_info,                  // Application Info
            0,                          // Layer count
            nullptr,                    // Layers
            extensions.size(),          // Extension count
            &extensions[0]              // Extensions
            );

        // Creating an instance
        return vk::createInstance(create_instance_info);
    }

    vk::PhysicalDevice pick_first_physical_device(const vk::Instance &instance) {
        std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
        assert(!physicalDevices.empty());

        return physicalDevices[0];
    }


    uint32_t pick_queue_family(const vk::PhysicalDevice &physical_device, const vk::QueueFlags flags) {

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physical_device.getQueueFamilyProperties();

        size_t queueFamilyIndex = std::distance(queueFamilyProperties.begin(),
                                                        std::find_if(queueFamilyProperties.begin(),
                                                                        queueFamilyProperties.end(),
                                                                        [flags](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & flags; }));

        assert(queueFamilyIndex < queueFamilyProperties.size());

        return static_cast<uint32_t>(queueFamilyIndex);
    }

    vk::Device create_device_khr(const vk::PhysicalDevice &physical_device, uint32_t queue_family) {
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

        return physical_device.createDevice(deviceCreateInfo);
    }

    std::tuple<glfw::GLFWwindow*, vk::SurfaceKHR> create_glfw_surface_khr(const vk::PhysicalDevice &physical_device, const vk::Instance &instance, uint32_t queue_family, const vk::Extent2D &surface_dimensions, const std::string &window_name) {
        if (surface_dimensions.width == 0 || surface_dimensions.height == 0) {
            throw std::runtime_error("Error: Window dimensions not set");
        }

        glfw::glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfw::GLFWwindow* window = glfw::glfwCreateWindow(surface_dimensions.width, surface_dimensions.height, window_name.c_str(), NULL, NULL);

        vk::SurfaceKHR surface;

        // Attach Vulkan Surface to GLFW window
        VkSurfaceKHR raw_surface(surface);
        VkResult result = glfw::glfwCreateWindowSurface(instance, window, NULL, &raw_surface);
        if (result != VK_SUCCESS) {
            std::cerr << "GLFW: Unable to create surface, error code " << static_cast<int>(result) << std::endl;
            exit(-1);
        }
        surface = (vk::SurfaceKHR)raw_surface;

        if (!physical_device.getSurfaceSupportKHR(queue_family, surface)) {
            std::cerr << "Surface not supported in queue family." << std::endl;
            exit(-1);
        }

        return std::tuple<glfw::GLFWwindow*, vk::SurfaceKHR>(window, surface);
    }

    vk::ShaderModule load_precompiled_shader(const vk::Device &device, const std::string &filename) {

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

    std::tuple<vk::SwapchainKHR, vk::Format> create_standard_swapchain(const vk::PhysicalDevice &physical_device, const vk::Device &device, const vk::SurfaceKHR &surface, vk::Extent2D dimensions, glfw::GLFWwindow *window, uint32_t queue_family) {
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
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            dimensions = capabilities.currentExtent;
        } else {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            dimensions.width = (uint32_t) w;
            dimensions.height = (int32_t) h;
        }
        
        uint32_t min_image_count = (capabilities.maxImageCount > 0) ? std::min(capabilities.maxImageCount, capabilities.minImageCount + 1) : capabilities.minImageCount + 1;

        vk::SwapchainCreateInfoKHR swap_chain_info(
            vk::SwapchainCreateFlagsKHR(),
            surface,                                    // SurfaceKHR
            min_image_count,                            // Min image count
            image_format,                               // Image format
            color_space,                                // Color space
            dimensions,                                 // Extent
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

        vk::SwapchainKHR swapchain = device.createSwapchainKHR(swap_chain_info);

        if (!swapchain) {
            throw std::runtime_error("Failed to create swapchain");
        }

        return std::tuple<vk::SwapchainKHR, vk::Format>(swapchain, image_format);

        }

    std::vector<vk::ImageView> create_swapchain_image_views(const vk::Device &device, const std::vector<vk::Image> &swapChainImages, const vk::Format &swapChainImageFormat) {
        std::vector<vk::ImageView> swapChainImageViews(swapChainImages.size());

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

        return swapChainImageViews;
    }
}