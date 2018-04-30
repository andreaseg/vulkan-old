#ifndef VULKAN_PRINT
#define VULKAN_PRINT

#include <iostream>

#include "util/console_color.hpp"

#include "includes.hpp"

void PrintPhysicalDevice(vk::PhysicalDevice &physical_device) {
    vk::PhysicalDeviceProperties properties = physical_device.getProperties();
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
    std::cout << std::dec;
    std::cout << "Device type: " << to_string(properties.deviceType) << std::endl;
    console_color::set_color(old_color);
}

void PrintQueueFamilyProperties(vk::QueueFamilyProperties &properties) {
    std::cout << "Family with " << properties.queueCount << " queues supporting features: " << std::endl;
    console_color::color old_color = console_color::set_color(console_color::GREY);
    std::cout << to_string(properties.queueFlags) << std::endl;
    console_color::set_color(old_color);
}

#endif