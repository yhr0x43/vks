#pragma once

#include <vulkan/vulkan.h>

#include "vks/Instance.hpp"
#include "vks/Device.hpp"
#include "vks/VulkanEncapsulate.hpp"

namespace vks
{
class Swapchain : public VulkanEncapsulate<VkSwapchainKHR>
{
	Instance const& m_Instance;
	Device const& m_Device;
	VkSurfaceKHR const& m_Surface;

	uint32_t m_QueueIndex;
	VkFormat m_ColorFormat;
	VkColorSpaceKHR m_ColorSpace;

	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_Views;

public:
	VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
	VkResult QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) const noexcept;
	void Recreate(uint32_t& width, uint32_t& height, bool vsync) noexcept;
	uint32_t const& GetQueueIndex(void) const noexcept;
	size_t GetImageCount(void) const noexcept;
	VkFormat const& GetColorFormat(void) const noexcept;
	std::vector<VkImageView> const& GetImageViews(void) const noexcept;

	Swapchain(
		Instance const& instance,
		Device const& device,
		VkSurfaceKHR surface,
		uint32_t& width, 
		uint32_t& height,
		bool vsync
	) noexcept;
	~Swapchain(void);
};
}