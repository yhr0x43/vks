#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vks/Buffer.hpp"
#include "vks/VulkanEncapsulate.hpp"

namespace vks
{
class Device : public VulkanEncapsulate<VkDevice>
{
	VkPhysicalDevice m_PhysicalDevice;
	VkPhysicalDeviceProperties m_Properties;
	VkPhysicalDeviceFeatures m_Features;
	VkPhysicalDeviceFeatures m_EnabledFeatures;
	VkPhysicalDeviceMemoryProperties m_MemoryProperties;
	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
	std::vector<std::string> m_SupportedExtensions;

	/* this command pool is created with graphics queue, for buffer operation that needs a command pool */
	VkCommandPool m_CmdPool;
	friend void Buffer::CopyFrom(Buffer& src, std::optional<VkBufferCopy> bufferCopy) const noexcept;

	std::optional<uint32_t> GetQueueFamilyIndex(VkQueueFlags queueFlags) const noexcept;

public:
	VkPhysicalDevice const& GetPhysicalDevice(void) const noexcept;
	bool ExtensionSupported(const std::string& name) const noexcept;
	void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue) const noexcept;
	std::optional<VkFormat> SupportedDepthStencilFormat(void) const noexcept;
	std::optional<uint32_t> GetMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties) const noexcept;

	struct
	{
		uint32_t Graphics;
		uint32_t Compute;
		uint32_t Transfer;
	} QueueIndex;

	Device(
		VkPhysicalDevice gpu,
		VkPhysicalDeviceFeatures enabledFeatures = {},
		std::vector<const char*> enabledExtensions = {},
		void* pNextChain = nullptr,
		VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
	) noexcept;
	~Device(void);
};
}