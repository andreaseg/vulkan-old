#ifndef GRAPHICS_ENGINE_HPP
#define GRAPHICS_ENGINE_HPP

#include "includes.hpp"
#include <algorithm>
#include <functional>
#include <unordered_map>

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

        void close();

        // Forbid copy
        Graphics(const Graphics&) = delete;

        ~Graphics();

        void addKeyCallback(int key, int action, int modifier, std::function<void()> callback);
        void addMouseCallback(int key, int action, int modifier, std::function<void(double xpos, double ypos)> callback);
        void addMousePosCallback(std::function<void(double xpos, double ypos)> callback);
        void addWindowFocusCallback(bool focus, std::function<void()> callback);
    private:
        static Graphics* current_engine;

        // GLFW
        glfw::GLFWwindow* window;

        bool has_focus;

        struct key_event {
            int action;
            int modifier;
            std::function<void()> callback;
        };

        struct mouse_event {
            int action;
            int modifier;
            std::function<void(double xpos, double ypos)> callback;
        };

        struct focus_event {
            bool focus;
            std::function<void()> callback;
        };
        
        std::unordered_multimap<int, key_event> key_callbacks;
        std::unordered_multimap<int, mouse_event> mouse_callbacks;
        std::vector<std::function<void(double xpos, double ypos)>> mouse_pos_callbacks;
        std::vector<focus_event> window_focus_callbacks;

        static void glfw_key_callback(glfw::GLFWwindow *window, int key, int scancode, int action, int mods);
        static void glfw_mouse_callback(glfw::GLFWwindow *window, int key, int action, int mods);
        static void glfw_mouse_pos_callback(glfw::GLFWwindow *window, double xpos, double ypos);
        static void glfw_window_focus_callback(glfw::GLFWwindow *window, int status);


        // Vulkan
        vk::Extent2D dimensions;

        vk::Instance instance;
        vk::PhysicalDevice physical_device;
        vk::Device device;
        uint32_t queue_family;
        vk::Queue queue;
        vk::SurfaceKHR surface;
        
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> swapChainImages;
        vk::Format swapChainImageFormat;
        std::vector<vk::ImageView> swapChainImageViews;

        vk::RenderPass renderPass;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline graphicsPipeline;

        std::vector<vk::Framebuffer> swapChainFrameBuffers;
        vk::CommandPool commandPool;
        std::vector<vk::CommandBuffer> commandBuffers;

        std::vector<vk::Semaphore> imageAvailableSemaphores;
        std::vector<vk::Semaphore> renderFinishedSemaphores;
        std::vector<vk::Fence> inFlightFences;
        size_t current_frame = 0;


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
        void create_framebuffers();
        void create_command_pool();
        void create_command_buffers();
        void create_sync_objects();


        void draw_frame();
        virtual void loop() = 0;
};

#endif //GRAPHICS_ENINGE_HPP