#include "vulkanLoader.h"

LIBRARY_TYPE load_vulkan() {

    // Loading library

    LIBRARY_TYPE vulkan_library;
    #ifdef __linux__
    vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW);
    #elif _WIN32
    vulkan_library = LoadLibrary("vulkan-1.dll");
    #else
    #error "Unknown or unsupported OS"
    #endif

    if (vulkan_library != nullptr) {
        console_color::color old_color = console_color::set_color(console_color::GREEN);
        std::cout << "Loaded Vulkan library" << std::endl;
        console_color::set_color(old_color);
    }
    else {
        console_color::color old_color = console_color::set_color(console_color::RED);
        std::cerr << "Failed to load Vulkan library" << std::endl;
        console_color::set_color(old_color);
        std::cerr << "Library file might be missing \n";
        #ifdef __linux__
        std::cerr << "Check if libvulkan.so.1 is in project directory, or VulkanSDK is present and in the enviroment as VULKAN_SDK";
        #elif _WIN32
        std::cerr << "Check if vulkan-1.dll is in project directory, or VulkanSDK is present and added to path as VULKAN_SDK";
        #else
        #error "Unknown or unsupported OS"
        #endif
        std::cerr << std::endl;
    }

    return vulkan_library;

}

bool LoadFunctionExportedFromVulkanLoaderLibrary( LIBRARY_TYPE const & vulkan_library ) {

    #if defined _WIN32
    #define LoadFunction GetProcAddress
    #elif defined __linux
    #define LoadFunction dlsym
    #endif

    #define EXPORTED_VULKAN_FUNCTION( name )                                    \
        name = (PFN_##name)LoadFunction( vulkan_library, #name );               \
        if( name == nullptr ) {                                                 \
            std::cerr << "Could not load exported Vulkan function named: "      \
            #name << std::endl;                                                 \
            return false;                                                       \
        } else {                                                                \
            std::cout << "Loaded Vulkan function named: " <<                    \
            #name << std::endl;                                                 \
        }

    #include "ListOfVulkanFunctions.inl"
    return true;
}

bool LoadGlobalFunctions() {

    #define GLOBAL_LEVEL_VULKAN_FUNCTION( name )                                \
        name = (PFN_##name)vkGetInstanceProcAddr( nullptr, #name );             \
        if( name == nullptr ) {                                                 \
            std::cerr << "Could not load global level Vulkan function named: "  \
            #name << std::endl;                                                 \
            return false;                                                       \
        } else {                                                                \
            std::cout << "Loaded Vulkan function named: " <<                    \
            #name << std::endl;                                                 \
        }

    

    #include "ListOfVulkanFunctions.inl"
    return true;
}

bool CheckAvailableInstanceExtensions(std::vector<VkExtensionProperties> &available_extensions) {

    uint32_t extension_count = 0;
    VkResult result = VK_SUCCESS;

    result = vkEnumerateInstanceExtensionProperties (nullptr, &extension_count, nullptr);
    if ( (result != VK_SUCCESS) ||
         (extension_count == 0)) {
             std::cerr << "Could not get number of extensions" << std::endl;
             return false;
         }
    
    std::cout << "Found " << extension_count << " instance extensions:" << std::endl;
    
    available_extensions.resize(extension_count);
    result = vkEnumerateInstanceExtensionProperties (nullptr, &extension_count, &available_extensions[0]);
    if ( (result != VK_SUCCESS) ||
         (extension_count == 0)) {
             std::cerr << "Could not enumerate Instance extensions" << std::endl;
             return false;
         }

    for (auto & e : available_extensions) {
        std::cout << " " << e.extensionName << std::endl;
    }

    return true;
}

bool IsExtensionSupported(std::vector<VkExtensionProperties> available_extensions, char* desired_extension) {
    for (auto & e : available_extensions) {
        if (strcmp(e.extensionName, desired_extension)) {
            return true;
        }
    }
    std::cerr << "Extension named '" << desired_extension << "' is not supported." << std::endl;
    return false;
}

bool CreateInstance(std::vector<char*> desired_extensions, VkInstance &instance) {
    std::vector<VkExtensionProperties> available_extensions;
    if (!CheckAvailableInstanceExtensions(available_extensions)) {
        return false;
    }

    for (auto& extension : desired_extensions) {
        if (!IsExtensionSupported(available_extensions, extension)) {
            return false;
        }
    }

    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "Vulkan cookbook",                          // Application name
        VK_MAKE_VERSION(1, 0, 0),                   // Application version
        "Vulkan cookbook (engine)",                 // Engine name
        VK_MAKE_VERSION(1, 0, 0),                   // Engine version
        VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION)    // Vulkan API version
    };

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &application_info,                                                  // Application info
        0,                                                                  // Layer count
        nullptr,                                                            // Layer names
        static_cast<uint32_t>(desired_extensions.size()),                   // Extension count
        desired_extensions.size() > 0 ? &desired_extensions[0] : nullptr    // Extension names
    };

    VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance);
    if ((result != VK_SUCCESS) || (instance == VK_NULL_HANDLE)) {
        std::cerr << "Could not create Vulkan instance: ";
        switch (result) {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            std::cerr << "Out of host memory.";
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            std::cerr << "Out of device memory";
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            std::cerr << "Initialization failed";
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            std::cerr << "Layer not present";
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            std::cerr << "Extension not present";
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            std::cerr << "Incompatible driver";
            break;
        default:
            std::cerr << "Unknown error";
            break;
        }
        std::cerr << std::endl;
        return false;
    }

    
    if (desired_extensions.size() > 0) {
        std::cout << "Created instance with extensions:" << std::endl;
        for (char* extension : desired_extensions) {
            std::cout << " " << extension << std::endl;
        }
    }
    else {
        std::cout << "Created instance without extensions" << std::endl;
    }
    

    return true;
}


bool LoadInstanceLevelFunctions(VkInstance &instance, std::vector<char*> enabled_extensions) {

    #define INSTANCE_LEVEL_VULKAN_FUNCTION( name )                         \
    name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);      \
    if (name == nullptr) {                                          \
        std::cerr << "Could not load instance-level function named "\
        #name << std::endl;                                         \
        return false;                                               \
    }                                                               \
    else {                                                          \
        std::cout << "Loaded instance-level function named "        \
        #name << std::endl;                                         \
    }

    #define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension )            \
    for(auto & enabled_extension : enabled_extensions) {                                \
        if(strcmp(enabled_extension, extension)) {                                      \
            name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);                  \
            if (name == nullptr) {                                                      \
                std::cerr << "Could not load instance-level function extension named "  \
                #name << std::endl;                                                     \
                return false;                                                           \
            }                                                                           \
            else {                                                                      \
                std::cout << "Loaded instance-level function extension named "          \
                #name << std::endl;                                                     \
            }                                                                           \
        }                                                                               \
    }

    #include "ListOfVulkanFunctions.inl"
    return true;
}

bool EnumeratePhysicalDevices(VkInstance &instance, std::vector<VkPhysicalDevice> &available_devices) {
    uint32_t device_count = 0;
    VkResult result = VK_SUCCESS;

    result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if ( (result != VK_SUCCESS) ||
        (device_count) == 0) {
            std::cerr << "Could not enumerate physical devices" << std::endl;
            return false;
        }

    available_devices.resize(device_count);
    result = vkEnumeratePhysicalDevices(instance, &device_count, &available_devices[0]);
    if ( (result != VK_SUCCESS) ||
        (device_count) == 0) {
            std::cerr << "Could not enumerate physical devices" << std::endl;
            return false;
        }

    std::cout << "Found " << device_count << " physical devices:" << std::endl;
    for (VkPhysicalDevice device : available_devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        console_color::color old_color = console_color::set_color(console_color::YELLOW);
        std::cout << properties.deviceName << std::endl;
        console_color::set_color(old_color);
        old_color = console_color::set_color(console_color::GREY);

        std::cout << "Vulkan API: " << VK_VERSION_MAJOR(properties.apiVersion) << "."
        << VK_VERSION_MINOR(properties.apiVersion) << "."
        << VK_VERSION_PATCH(properties.apiVersion) << "\n";
        std::cout << "Driver: " << properties.driverVersion << "\n";
        std::cout << "VendorID: 0x" << std::hex << properties.vendorID << "\n";
        std::cout << "DeviceID: 0x" << std::hex << properties.deviceID << "\n";

        std::cout << "Device type: ";
        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            std::cout << "Integrated GPU";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            std::cout << "Discrete GPU";
            break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            std::cout << "Virtual GPU";
            break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
            std::cout << "CPU";
            break;
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            std::cout << "Other";
            break;
            default:
            std::cout << "Unknown";
        }
        std::cout << "\n";

        std::cout << std::endl;
        console_color::set_color(old_color);
    }

    return true;
}