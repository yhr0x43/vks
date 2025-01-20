#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>

#if !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif // _MSC_VER

#define VK_CHK(x) do {                                           \
VkResult ret = (x);                                              \
if (VK_SUCCESS != ret) {                                         \
	spdlog::critical("\"{}\" results {} in {}",                  \
		#x, vks::utils::statusString(ret), __PRETTY_FUNCTION__); \
	assert(ret == VK_SUCCESS);                                   \
} } while(0)                                                     \

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

namespace vks
{
/**
* utils namespace contains function not directly associated with any Vulkan concepts, but is convienient to have
*/
namespace utils
{
std::string statusString(VkResult result) noexcept;
void exitFatal(const std::string& message, int32_t exitCode);
void exitFatal(const std::string& message, VkResult resultCode);
VkShaderModule loadShader(const char* fileName, VkDevice device);
}
}