#pragma once
#include <string>

#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

namespace vks
{
namespace debug
{
void setupDebugging(VkInstance instance);
void freeDebugCallback(VkInstance instance);
void setupDebugingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCI);
}
namespace debugutils
{
void setup(VkInstance instance);
void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color);
void cmdEndLabel(VkCommandBuffer cmdbuffer);
}
}