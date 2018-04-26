#include <iostream>
#include <string>

#define VULKAN_HPP_NO_SMART_HANDLE
#include "vulkan/vulkan.hpp"
#include "vulkan_print.hpp"

static const char* AppName = "App name";
static const char* EngineName = "Engine name";

int main(int argc, char** argv) {
    
    vk::ApplicationInfo appInfo(AppName, 1, EngineName, 1, VK_API_VERSION_1_1);
    vk::Instance instance = vk::createInstance(vk::InstanceCreateInfo({}, &appInfo));
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
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
    vk::Device device = physicalDevices[0].createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo));

    return 0;
    
}
