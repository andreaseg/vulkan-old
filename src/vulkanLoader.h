#ifndef VULKAN_LOADER_H
#define VULKAN_LOADER_H

#include <iostream>
#include <vector>

#include "config_loader.h"

#ifdef __linux__
#include <dlfcn.h>
#elif _WIN32
#include <Windows.h>
#else
#error "Unknown or unsupported OS"
#endif

#define VK_NO_PROTOTYPES
#include "vulkan.h"

// Vulkan library type
#ifdef __linux__
#define LIBRARY_TYPE void*
#elif _WIN32
#define LIBRARY_TYPE HMODULE
#else
#error "Unknown or unsupported OS"
#endif

LIBRARY_TYPE load_vulkan();

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

bool LoadFunctionExportedFromVulkanLoaderLibrary( LIBRARY_TYPE const & vulkan_library );

static PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
static PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
static PFN_vkCreateInstance vkCreateInstance;

bool LoadGlobalFunctions();
    
bool CheckAvailableInstanceExtensions(std::vector<VkExtensionProperties> &available_extensions);

bool CreateInstance(std::vector<char*> desired_extensions, VkInstance &instance);

static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
static PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
static PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
static PFN_vkCreateDevice vkCreateDevice;
static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

static PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;

#ifdef VK_USE_PLATFORM_WIN32_KHR
PFN_vkCreateWin32SurfaceKHR = vkCreateWin32SurfaceKHR;
#elif defined VK_USE_PLATFORM_XCB_KHR
PFN_vkCreateXcbSurfaceKHR = vkCreateXcbSurfaceKHR;
#elif defined VK_USE_PLATFORM_XLIB_KHR
PFN_vkCreateXlibSurfaceKHR = vkCreateXlibSurfaceKHR;
#endif

bool LoadInstanceLevelFunctions(VkInstance &instance, std::vector<char*> enabled_extensions);

#endif