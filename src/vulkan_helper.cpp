#include "vulkan_helper.hpp"
#include <iostream>

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

    
}