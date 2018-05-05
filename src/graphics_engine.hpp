#ifndef GRAPHICS_ENGINE_HPP
#define GRAPHICS_ENGINE_HPP

#include "includes.hpp"

class Graphics {
    public:
        vk::Instance instance;
        vk::PhysicalDevice physical_device;
        vk::Device device;
        uint32_t queue_family;
        vk::Queue queue;
        vk::SurfaceKHR surface;
        glfw::GLFWwindow* window;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> swapChainImages;
        vk::Format swapChainImageFormat;

        const char* AppName = "App name";
        const char* EngineName = "Engine name";

        int width = 640;
        int height = 480;

        Graphics();

        void start();

        ~Graphics();
    private:
        void check_support();
        vk::ApplicationInfo generate_app_info();
        void create_instance(vk::ApplicationInfo app_info);
        void pick_physical_device();
        void pick_queue_family(vk::QueueFlags flags);
        void create_logical_device();
        void create_surface();
        void create_swapchain();

        virtual void loop() = 0;
};

#endif //GRAPHICS_ENINGE_HPP