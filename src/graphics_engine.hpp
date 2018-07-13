#ifndef GRAPHICS_ENGINE_HPP
#define GRAPHICS_ENGINE_HPP

#include "includes.hpp"
#include "vulkan_memory.hpp"
#include "vulkan_helper.hpp"
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <array>

struct Transformations {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
};

struct FrameSyncObjects {
    vk::Fence inFlightFence;
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
};

class Graphics {
    public:
        const char* AppName = "Vulkan Tutorial";
        const char* EngineName = "Vulkan Engine";

        uint32_t getWidth();
        uint32_t getHeight();
        float getAspectRatio();

        void setDimensions(uint32_t width, uint32_t height);

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

        bool has_been_resized;

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
        static void window_size_callback(glfw::GLFWwindow *window, int width, int height);


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

        vk::DescriptorPool descriptorPool;
        vk::DescriptorSetLayout descriptorSetLayout;
        std::vector<vk::DescriptorSet> descriptorSets;

        vk::PipelineLayout pipelineLayout;
        vk::Pipeline graphicsPipeline;

        std::vector<vk::Framebuffer> swapChainFrameBuffers;
        vk::CommandPool commandPool;
        std::vector<vk::CommandBuffer> commandBuffers;

        std::vector<FrameSyncObjects> frameSyncObjects;
        size_t current_frame = 0;

        vk_mem::Manager memoryManager;
        vk_mem::BufferHandle vertexBuffer;
        vk_mem::BufferHandle indexBuffer;
        std::vector<vk_mem::BufferHandle> uniformBuffers;


        void check_support();
        void create_instance();
        void pick_physical_device();
        void pick_queue_family();
        void create_logical_device();
        void create_surface();
        void create_swapchain();
        void create_image_views();
        void create_render_pass();
        void create_descriptor_set_layout();
        void create_pipeline();
        void create_framebuffers();
        void create_command_pool();
        void create_vertex_buffers();
        void create_index_buffers();
        void create_uniform_buffers();
        void create_descriptor_pool();
        void create_descriptor_set();
        void create_command_buffers();
        void create_sync_objects();

        void recreate_swapchain();
        void clean_up_swapchain();

        void update_uniform_buffers(uint32_t image_index);
        void draw_frame();
        virtual void loop() = 0;
};

#endif //GRAPHICS_ENINGE_HPP