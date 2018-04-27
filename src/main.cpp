#include <iostream>
#include <string>

#include "vulkan/vulkan.hpp"
#include "vulkan_print.hpp"

static const char* AppName = "App name";
static const char* EngineName = "Engine name";

int main(int argc, char** argv) {

    try{
    
        vk::ApplicationInfo appInfo(AppName, 1, EngineName, 1, VK_API_VERSION_1_1);
        vk::UniqueInstance instance = vk::createInstanceUnique(vk::InstanceCreateInfo({}, &appInfo));
        std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
        assert(!physicalDevices.empty());

        PrintPhysicalDevice(physicalDevices[0]);

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevices[0].getQueueFamilyProperties();

        std::cout << "Found " << queueFamilyProperties.size() << " queue families." << std::endl;
        for (vk::QueueFamilyProperties & property : queueFamilyProperties) {
            PrintQueueFamilyProperties(property);
        }

        size_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(),
                                                        std::find_if(queueFamilyProperties.begin(),
                                                                        queueFamilyProperties.end(),
                                                                        [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));
        assert(graphicsQueueFamilyIndex < queueFamilyProperties.size());

        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(graphicsQueueFamilyIndex), 1, &queuePriority);
        vk::UniqueDevice device = physicalDevices[0].createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo));

    }
    catch (vk::SystemError err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    }
    catch (...) {
        std::cout << "unknown error\n";
        exit(-1);
    }

    return 0;
    
}
