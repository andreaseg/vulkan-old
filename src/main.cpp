#include <iostream>

#include "vulkanLoader.h"

int main(int argc, char** argv) {
    LIBRARY_TYPE vulkan_library = load_vulkan();
    LoadFunctionExportedFromVulkanLoaderLibrary(vulkan_library);
    LoadGlobalFunctions();
    
    VkInstance instance;
    std::vector<char*> desired_extensions;// = {(char*)"VK_KHR_win32_surface"};
    CreateInstance(instance, desired_extensions);

    std::vector<char*> enabled_extensions;
    LoadInstanceLevelFunctions(instance, enabled_extensions);

    std::vector<VkPhysicalDevice> available_devices;
    EnumeratePhysicalDevices(instance, available_devices);
    PrintPhysicalDevice(available_devices[0]);

    std::vector<VkExtensionProperties> available_extensions;
    CheckAvailableDeviceExtensions(available_devices[0], available_extensions);

    std::vector<VkQueueFamilyProperties> queue_families;

    CheckAvailableQueueFamilies(available_devices[0], queue_families);

    std::cout << "Found " << queue_families.size() << " queue families:" << std::endl;
    for (auto queue : queue_families) {
        PrintQueueFamilyProperties(queue);
    }

    uint32_t queue_family_index = -1;

    VkQueueFlags desired_capabilities = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;

    SelectQueueFamily(available_devices[0], desired_capabilities, queue_family_index);

    std::cout << "Selected queue family with index " << queue_family_index << std::endl;

    std::vector<QueueInfo> queue_infos = {QueueInfo()};
    queue_infos[0].FamilyIndex = queue_family_index;
    queue_infos[0].Priorities = {1.0, 1.0, 1.0, 1.0};
    VkPhysicalDeviceFeatures desired_features;
    VkDevice logical_device;
    
    if (CreateLogicalDevice(available_devices[0], queue_infos, desired_extensions, desired_features, logical_device)) {
        console_color::color old_color = console_color::set_color(console_color::GREEN);
        std::cout << "Created logical device";
        console_color::set_color(console_color::GREY);

        if (desired_extensions.size() > 0) {
            std::cout << " with extensions:" << std::endl;
            for(char* extension : desired_extensions) {
                std::cout << " " << extension << std::endl;
            }
        }
        else {
            std::cout << " without extensions" << std::endl;
        }

        console_color::set_color(old_color);
    }

    
}
