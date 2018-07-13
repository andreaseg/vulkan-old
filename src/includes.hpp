#ifndef PROJECT_INCLUDES_HPP
#define PROJECT_INCLUDES_HPP

#define NOMINMAX

#define  VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
namespace glfw{
    #define GLFW_DLL
    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
}
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#endif