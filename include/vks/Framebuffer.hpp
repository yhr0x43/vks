#pragma once

#include <vulkan/vulkan.h>

#include "vks/Device.hpp"
#include "vks/VulkanEncapsulate.hpp"

namespace vks
{
class FramebufferAttachment : public NonCopyable
{
	Device const& m_Device;

	VkImageCreateInfo m_ImageCreateInfo;
	VkMemoryAllocateInfo m_MemoryAllocateInfo;
	VkImageViewCreateInfo m_ImageViewCreateInfo;

	VkImage m_Image;
	VkImageView m_ImageView;
	VkDeviceMemory m_Memory;
	VkAttachmentDescription m_Description;

	void Init(void);
	void Destroy(void);

public:
	void Recreate(VkExtent3D extent) noexcept;
	VkImageView const& GetView(void) const noexcept;

	FramebufferAttachment(
		Device const& device,
		VkImageCreateInfo const& imageCI,
		VkImageViewCreateInfo& imageViewCI
	) noexcept;
	~FramebufferAttachment(void) noexcept;
};
}