#ifndef vulkan_helper_hpp
#define vulkan_helper_hpp

#define  VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
namespace glfw{
    #define GLFW_DLL
    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
}

namespace vk_help {
    vk::Instance create_glfw_instance(const std::string &app_name, const std::string &engine_name, const std::vector<char const*> *optional_extensions = nullptr);

    vk::PhysicalDevice pick_first_physical_device(const vk::Instance &instance);

    uint32_t pick_queue_family(const vk::PhysicalDevice &physical_device, const vk::QueueFlags flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);

    vk::Device create_device_khr(const vk::PhysicalDevice &physical_device, uint32_t queue_family);

    std::tuple<glfw::GLFWwindow*, vk::SurfaceKHR> create_glfw_surface_khr(const vk::PhysicalDevice &physical_device, const vk::Instance &instance, uint32_t queue_family, const vk::Extent2D &surface_dimensions, const std::string &window_name);
}

#endif // vulkan_helper_hpp