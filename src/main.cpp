#include <iostream>

#include "vulkanLoader.h"

int main(int argc, char** argv) {
    LIBRARY_TYPE vulkan_library = load_vulkan();
    LoadFunctionExportedFromVulkanLoaderLibrary(vulkan_library);
    LoadGlobalFunctions();
    
    VkInstance instance;
    std::vector<char*> desired_extensions = {(char*)"VK_KHR_win32_surface"};
    CreateInstance(desired_extensions, instance);

    std::vector<char*> enabled_extensions;

    LoadInstanceLevelFunctions(instance, enabled_extensions);
}
