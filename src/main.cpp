#include <iostream>
#include <string>

#include "vulkan/vulkan.hpp"
#include "vulkan_print.hpp"

static const char* AppName = "App name";
static const char* EngineName = "Engine name";

class Graphics {
    public:
        vk::UniqueInstance instance;
        vk::UniqueDevice device;
        vk::Queue queue;
        vk::SurfaceKHR surface;

        Graphics() {
            try{
                // Creating an instance

                vk::ApplicationInfo appInfo(AppName, 1, EngineName, 1, VK_API_VERSION_1_1);
                instance = vk::createInstanceUnique(vk::InstanceCreateInfo({}, &appInfo));

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

                // Creating a surface

            }
            catch (vk::SystemError err) {
                std::cout << "vk::SystemError: " << err.what() << std::endl;
                exit(-1);
            }
            catch (...) {
                std::cout << "unknown error\n";
                exit(-2);
            }
        }
    private:
};

int main(int argc, char** argv) {

    #ifdef DEBUG
        std::cout << "Running in Debug mode" << std::endl;
    #endif

    // Parameters
    for(int i = 1; i < argc; i++) {
        char* argument = argv[i];
        if(strcmp(argument, "-h") || strcmp(argument, "help")) {
            std::cout << "This program takes no arguments." << std::endl;
            return 0;
        }
    }

    Graphics gfx;

    return 0;
    
}
