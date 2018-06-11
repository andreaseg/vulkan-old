#ifndef GRAPHICS_ENGINE_HPP
#define GRAPHICS_ENGINE_HPP

#include "includes.hpp"

class Graphics {
    public:
        const char* AppName = "App name";
        const char* EngineName = "Engine name";

        uint32_t getWidth();
        uint32_t getHeight();

        void setDimensions(uint32_t width, uint32_t height);

        vk::ShaderModule load_precompiled_shader(const std::string &filename);

        Graphics();

        void start();

        ~Graphics();
    private:
        vk::Extent2D dimensions;

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
        std::vector<vk::ImageView> swapChainImageViews;
        vk::RenderPass renderPass;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline graphicsPipeline;

        void check_support();
        vk::ApplicationInfo generate_app_info();
        void create_instance(vk::ApplicationInfo app_info);
        void pick_physical_device();
        void pick_queue_family(vk::QueueFlags flags);
        void create_logical_device();
        void create_surface();
        void create_swapchain();
        void create_image_views();
        void create_render_pass();
        void create_pipeline();

        virtual void loop() = 0;
};

#endif //GRAPHICS_ENINGE_HPP