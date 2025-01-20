#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vks/VulkanEncapsulate.hpp"

namespace vks
{
/**
* Instance class
* @brief represents one Vulkan instance
*/
class Instance : public VulkanEncapsulate<VkInstance>
{
	/** @brief fetched supported instance extensions */
	std::vector<std::string> m_SupportedExtensions;

public:
	bool ExtensionSupported(const std::string& name);

	Instance(
		VkApplicationInfo appInfo,
		bool validation = true,
		std::vector<const char*> enabledExtensions = {}
	) noexcept;
	~Instance(void);
};
}