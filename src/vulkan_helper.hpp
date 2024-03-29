#ifndef VULKAN_HELPER_HPP
#define VULKAN_HELPER_HPP

#include "includes.hpp"

namespace vk_help {
    vk::Instance create_glfw_instance(const std::string &app_name, const std::string &engine_name, const std::vector<char const*> optional_extensions = {});

    vk::PhysicalDevice pick_first_physical_device(const vk::Instance &instance);

    uint32_t pick_queue_family(const vk::PhysicalDevice &physical_device, const vk::QueueFlags flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);

    vk::Device create_device_khr(const vk::PhysicalDevice &physical_device, uint32_t queue_family);

    std::tuple<glfw::GLFWwindow*, vk::SurfaceKHR> create_glfw_surface_khr(const vk::PhysicalDevice &physical_device, const vk::Instance &instance, uint32_t queue_family, const vk::Extent2D &surface_dimensions, const std::string &window_name);

    vk::ShaderModule load_precompiled_shader(const vk::Device &device, const std::string &filename);

    std::tuple<vk::SwapchainKHR, vk::Format> create_standard_swapchain(const vk::PhysicalDevice &physical_device, const vk::Device &device, const vk::SurfaceKHR &surface, vk::Extent2D dimensions, glfw::GLFWwindow *window, uint32_t queue_family);

    std::vector<vk::ImageView> create_swapchain_image_views(const vk::Device &device, const std::vector<vk::Image> &swapChainImages, const vk::Format &swapChainImageFormat);

    struct image_container {
        int width, height, channels;
        stbi_uc *image;

        image_container() {}

        ~image_container() {
            stbi_image_free(image);
        }

        image_container(image_container&&) = default;
        image_container& operator=(image_container&&) = default;

        image_container(const image_container&) = delete;
    };

    image_container load_image(std::string path, int format = STBI_rgb_alpha);
}

#endif // VULKAN_HELPER_HPP