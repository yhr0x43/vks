#pragma once

#include <vulkan/vulkan.h>

#include "vks/VulkanEncapsulate.hpp"

namespace vks
{
class Device;

class Buffer : public VulkanEncapsulate<VkBuffer>
{
	Device const& m_Device;

	VkDeviceMemory m_Memory;
	VkDeviceSize m_Size;
	void* m_pMapped;

public:
	VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const noexcept;
	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0, VkMemoryMapFlags flags = 0) noexcept;
	void Unmap(void) noexcept;
	void CopyData(const void* data, size_t size) noexcept;
	void CopyFrom(Buffer& src, std::optional<VkBufferCopy> bufferCopy = std::nullopt) const noexcept;

	Buffer(
		Device const& device,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize size,
		void* data = nullptr
	) noexcept;
	~Buffer(void) noexcept;
};
}