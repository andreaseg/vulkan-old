#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <algorithm>

#include "includes.hpp"
#include "vulkan_print.hpp"



class Graphics {
    public:
        vk::Instance instance;
        vk::Device device;
        vk::Queue queue;
        vk::SurfaceKHR surface;
        GLFWwindow* window;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> swapChainImages;
        vk::Format swapChainImageFormat;

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
                instance = vk::createInstance(createInstanceInfo);

                // Picking a physical device

                std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
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
                vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo);

                std::vector<char const*> device_level_extensions;
                device_level_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

                deviceCreateInfo.enabledExtensionCount = device_level_extensions.size();
                deviceCreateInfo.ppEnabledExtensionNames = &device_level_extensions[0];
                device = physicalDevices[0].createDevice(deviceCreateInfo);

                // Picking a queue

                queue = device.getQueue(queueFamilyIndex,0);

                // Create a GLFW Window

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

                if (!physicalDevices[0].getSurfaceSupportKHR(queueFamilyIndex, surface)) {
                    std::cerr << "Surface not supported in queue family." << std::endl;
                    exit(-1);
                }

                // Making a swap-chain

                vk::SwapchainCreateInfoKHR swapChainInfo;

                // Selecting format for swapchain
                auto surface_formats = physicalDevices[0].getSurfaceFormatsKHR(surface);
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
                auto presentation_modes = physicalDevices[0].getSurfacePresentModesKHR(surface);
                if (std::any_of(presentation_modes.begin(), presentation_modes.end(), [](vk::PresentModeKHR const& p){return p == vk::PresentModeKHR::eMailbox;})) {
                    swapChainInfo.presentMode = vk::PresentModeKHR::eMailbox;
                } else if (std::any_of(presentation_modes.begin(), presentation_modes.end(), [](vk::PresentModeKHR const& p){return p == vk::PresentModeKHR::eImmediate;})) {
                    swapChainInfo.presentMode = vk::PresentModeKHR::eImmediate;
                } else {
                    swapChainInfo.presentMode = vk::PresentModeKHR::eFifo;
                }

                // Selecting dimensions
                auto capabilities = physicalDevices[0].getSurfaceCapabilitiesKHR(surface);
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
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            vkDestroySurfaceKHR((VkInstance)instance, VkSurfaceKHR(surface), nullptr);
            device.destroy();
            instance.destroy();
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
    #else
    std::ofstream cout("log.txt");
    std::cout.rdbuf(cout.rdbuf());
    std::ofstream cerr("error.txt");
    std::cerr.rdbuf(cerr.rdbuf());
    #endif
    

    Graphics gfx;

    return 0;
    
}

// Windows entry-point
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return main();
}