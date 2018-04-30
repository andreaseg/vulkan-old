#include <iostream>
#include <string>
#include <cstdio>

#include "includes.hpp"
#include "vulkan_print.hpp"



class Graphics {
    public:
        vk::UniqueInstance instance;
        vk::UniqueDevice device;
        vk::Queue queue;
        vk::SurfaceKHR surface;
        GLFWwindow* window;

        const char* AppName = "App name";
        const char* EngineName = "Engine name";

        int width = 640;
        int height = 480;

        Graphics() {
            try{
                if (!glfwInit()) {
		            std::cerr << "GLFW not initialized." << std::endl;
                    exit(-1);
                }
                if (!glfwVulkanSupported()) {
                    std::cerr << "Vulkan not supported." << std::endl;
                    exit(-1);
                }
                // Application information
                vk::ApplicationInfo appInfo(AppName, 1, EngineName, 1, VK_API_VERSION_1_1);

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

                vk::InstanceCreateInfo createInstanceInfo = vk::InstanceCreateInfo({}, &appInfo);
                createInstanceInfo.enabledExtensionCount = extensions.size();
                createInstanceInfo.ppEnabledExtensionNames = &extensions[0];

                // Creating an instance
                instance = vk::createInstanceUnique(createInstanceInfo);

                // Picking a physical device

                std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
                assert(!physicalDevices.empty());

                PrintPhysicalDevice(physicalDevices[0]);

                // Picking a queue family

                std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevices[0].getQueueFamilyProperties();

                std::cout << "Found " << queueFamilyProperties.size() << " queue families." << std::endl;
                for (vk::QueueFamilyProperties & property : queueFamilyProperties) {
                    PrintQueueFamilyProperties(property);
                }

                size_t queueFamilyIndex = std::distance(queueFamilyProperties.begin(),
                                                                std::find_if(queueFamilyProperties.begin(),
                                                                                queueFamilyProperties.end(),
                                                                                [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics && qfp.queueFlags & vk::QueueFlagBits::eCompute; }));
                assert(queueFamilyIndex < queueFamilyProperties.size());

                // Creating a device

                float queuePriority = 0.0f;
                vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriority);
                device = physicalDevices[0].createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo));

                // Picking a queue

                queue = device->getQueue(queueFamilyIndex,0);

                // Create a GLFW Window

                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                window = glfwCreateWindow(width, height, AppName, NULL, NULL);

                // Attach Vulkan Surface to GLFW window
                VkSurfaceKHR raw_surface(surface);
                VkResult result = glfwCreateWindowSurface(instance.get(), window, NULL, &raw_surface);
                if (result != VK_SUCCESS) {
                    std::cerr << "GLFW: Unable to create surface, error code " << static_cast<int>(result) << std::endl;
                    exit(-1);
                }


                // Start the core-loop
                loop();

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

        ~Graphics() {
            vkDestroySurfaceKHR((VkInstance)instance.get(), VkSurfaceKHR(surface), nullptr);
            glfwDestroyWindow(window); 
            glfwTerminate();
        }
    private:

        void loop() {
            while (!glfwWindowShouldClose(window)) {
                glfwPollEvents();
            }
        }
};

int main() {

    #ifdef DEBUG
        std::cout << "Running in Debug mode" << std::endl;
        std::freopen( "log.txt", "w", stdout );
        std::freopen( "error.txt", "w", stderr );
    #endif

    Graphics gfx;

    return 0;
    
}

// Windows entry-point
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main();
}