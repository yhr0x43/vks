#include <array>

#include "vks/Utils.hpp"
#include "vks/Swapchain.hpp"

namespace vks
{
Swapchain::Swapchain(
	Instance const& instance,
	Device const& device,
	VkSurfaceKHR surface,
	uint32_t& width,
	uint32_t& height,
	bool vsync
) noexcept
	: m_Instance(instance), m_Device(device), m_Surface(surface)
{
	// Get available queue family properties
	uint32_t queueCnt;
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueCnt, NULL);
	if (1 > queueCnt)
	{
		vks::utils::exitFatal("no queue family properties found", -1);
	}

	std::vector<VkQueueFamilyProperties> queueProps(queueCnt);
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueCnt, queueProps.data());

	/* search for a queue with both graphics and present support */
	m_QueueIndex = UINT32_MAX;

	for (uint32_t i = 0; i < queueCnt; i++)
	{
		VkBool32 supportsPresent;
		vkGetPhysicalDeviceSurfaceSupportKHR(device.GetPhysicalDevice(), i, surface, &supportsPresent);
		if (supportsPresent && (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			m_QueueIndex = i;
			break;
		}
	}
	if (UINT32_MAX == m_QueueIndex)
	{
		vks::utils::exitFatal("no suitable swapchain queue found", -1);
	}

	uint32_t formatCnt;
	VK_CHK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface, &formatCnt, NULL));
	if (1 > formatCnt)
	{
		vks::utils::exitFatal("no suitable swapchain queue found", -1);
	}

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCnt);
	VK_CHK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface, &formatCnt, surfaceFormats.data()));

	VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
	std::array<VkFormat, 3> preferredImageFormats = {
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32
	};

	for (auto& availableFormat : surfaceFormats) {
		if (preferredImageFormats.end() != std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format)) {
			selectedFormat = availableFormat;
			break;
		}
	}

	m_ColorFormat = selectedFormat.format;
	m_ColorSpace = selectedFormat.colorSpace;

	Recreate(width, height, vsync);
}

Swapchain::~Swapchain(void)
{
	for (uint32_t i = 0; i < m_Images.size(); i++)
	{
		vkDestroyImageView(m_Device, m_Views[i], nullptr);
	}
	vkDestroySwapchainKHR(m_Device, m_Handle, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}

void Swapchain::Recreate(uint32_t& width, uint32_t& height, bool vsync) noexcept
{
	VkSurfaceCapabilitiesKHR surfCaps;
	VK_CHK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), m_Surface, &surfCaps));

	uint32_t presentModeCount;
	VK_CHK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Surface, &presentModeCount, NULL));
	assert(presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data()));

	VkExtent2D swapchainExtent = {};
	// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
	if (surfCaps.currentExtent.width == (uint32_t)-1)
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = width;
		swapchainExtent.height = height;
	}
	else
	{
		// If the surface size is defined, the swap chain size must match
		swapchainExtent = surfCaps.currentExtent;
		width = surfCaps.currentExtent.width;
		height = surfCaps.currentExtent.height;
	}


	// Select a present mode for the swapchain

	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// If v-sync is not requested, try to find a mailbox mode
	// It's the lowest latency non-tearing present mode available
	if (!vsync)
	{
		for (size_t i = 0; i < presentModeCount; i++)
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	// Determine the number of images
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// Find the transformation of the surface
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfCaps.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface = m_Surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = m_ColorFormat;
	swapchainCI.imageColorSpace = m_ColorSpace;
	swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.queueFamilyIndexCount = 0;
	swapchainCI.presentMode = swapchainPresentMode;
	// Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
	swapchainCI.oldSwapchain = m_Handle;
	// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
	swapchainCI.clipped = VK_TRUE;
	swapchainCI.compositeAlpha = compositeAlpha;

	// Enable transfer source on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VK_CHK(vkCreateSwapchainKHR(m_Device, &swapchainCI, nullptr, &m_Handle));

	// If an existing swap chain is re-created, destroy the old swap chain
	// This also cleans up all the presentable images
	/* cleanup for the old swapchain */
	if (swapchainCI.oldSwapchain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			vkDestroyImageView(m_Device, m_Views[i], nullptr);
		}
		vkDestroySwapchainKHR(m_Device, swapchainCI.oldSwapchain, nullptr);
	}

	uint32_t imageCnt;
	VK_CHK(vkGetSwapchainImagesKHR(m_Device, m_Handle, &imageCnt, NULL));

	// Get the swap chain images
	m_Images.resize(imageCnt);
	VK_CHK(vkGetSwapchainImagesKHR(m_Device, m_Handle, &imageCnt, m_Images.data()));

	// Get the swap chain buffers containing the image and imageview
	m_Views.resize(imageCnt);
	for (uint32_t i = 0; i < imageCnt; i++)
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = m_ColorFormat;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;

		colorAttachmentView.image = m_Images[i];

		VK_CHK(vkCreateImageView(m_Device, &colorAttachmentView, nullptr, &m_Views[i]));
	}
}

VkResult Swapchain::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
{
	return vkAcquireNextImageKHR(m_Device, m_Handle, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

VkResult  Swapchain::QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) const noexcept
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Handle;
	presentInfo.pImageIndices = &imageIndex;
	// Check if a wait semaphore has been specified to wait for before presenting the image
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		presentInfo.pWaitSemaphores = &waitSemaphore;
		presentInfo.waitSemaphoreCount = 1;
	}
	return vkQueuePresentKHR(queue, &presentInfo);
}

uint32_t const& Swapchain::GetQueueIndex(void) const noexcept
{
	return m_QueueIndex;
}

size_t Swapchain::GetImageCount(void) const noexcept
{
	return m_Images.size();
}
VkFormat const& Swapchain::GetColorFormat(void) const noexcept
{
	return m_ColorFormat;
}

std::vector<VkImageView> const& Swapchain::GetImageViews(void) const noexcept
{
	return m_Views;
}
}