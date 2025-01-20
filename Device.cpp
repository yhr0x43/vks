#include <array>

#include "vks/Utils.hpp"
#include "vks/Inits.hpp"

#include "vks/Device.hpp"

namespace vks
{
Device::Device(
	VkPhysicalDevice gpu,
	VkPhysicalDeviceFeatures enabledFeatures,
	std::vector<const char*> enabledExtensions,
	void* pNextChain,
	VkQueueFlags requestedQueueTypes
) noexcept
{
	m_PhysicalDevice = gpu;

	// Store Properties features, limits and properties of the physical device for later use
	// Device properties also contain limits and sparse properties
	vkGetPhysicalDeviceProperties(gpu, &m_Properties);
	// Features should be checked by the examples before using them
	vkGetPhysicalDeviceFeatures(gpu, &m_Features);
	// Memory properties are used regularly for creating all kinds of buffers
	vkGetPhysicalDeviceMemoryProperties(gpu, &m_MemoryProperties);
	// Queue family properties, used for setting up requested queues upon device creation
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
	m_QueueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, m_QueueFamilyProperties.data());

	/* get list of supported extensions */
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		VK_CHK(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extCount, extensions.data()));
		for (auto& ext : extensions)
		{
			m_SupportedExtensions.push_back(ext.extensionName);
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	const float queuePriority = 0.0f;

	
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		QueueIndex.Graphics = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT).value();
		queueCreateInfos.push_back(vks::inits::deviceQueueCreateInfo({}, QueueIndex.Graphics, 1, &queuePriority));
	}
	else
	{
		QueueIndex.Graphics = 0;
	}

	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		QueueIndex.Compute = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT).value();
		if (QueueIndex.Compute != QueueIndex.Graphics)
		{
			queueCreateInfos.push_back(vks::inits::deviceQueueCreateInfo({}, QueueIndex.Compute, 1, &queuePriority));
		}
	}
	else
	{
		/* compute queue not requested, use the same last queue */
		QueueIndex.Compute = QueueIndex.Graphics;
	}

	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		QueueIndex.Transfer = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT).value();
		if ((QueueIndex.Transfer != QueueIndex.Graphics) && (QueueIndex.Transfer != QueueIndex.Compute))
		{
			queueCreateInfos.push_back(vks::inits::deviceQueueCreateInfo({}, QueueIndex.Transfer, 1, &queuePriority));
		}
	}
	else
	{
		/* transfer queue not requested, use the same last queue */
		QueueIndex.Transfer = QueueIndex.Graphics;
	}

	std::vector<const char *> exts(enabledExtensions);
	exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
	if (pNextChain) {
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = enabledFeatures;
		physicalDeviceFeatures2.pNext = pNextChain;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		deviceCreateInfo.pNext = &physicalDeviceFeatures2;
	}

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_KHR_portability_subset)
	// SRS - When running on iOS/macOS with MoltenVK and VK_KHR_portability_subset is defined and supported by the device, enable the extension
	if (extensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	}
#endif
	for (auto const& ext : exts)
	{
		if (!ExtensionSupported(ext)) {
			spdlog::error("Enabled device extension {} is not present at device level", ext);
		}
	}

	deviceCreateInfo.enabledExtensionCount = (uint32_t)exts.size();
	deviceCreateInfo.ppEnabledExtensionNames = exts.data();

	m_EnabledFeatures = enabledFeatures;

	VK_CHK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Handle));

	VkCommandPoolCreateInfo cmdPoolInfo = vks::inits::commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = QueueIndex.Graphics;
	VK_CHK(vkCreateCommandPool(m_Handle, &cmdPoolInfo, nullptr, &m_CommandPool));
}

Device::~Device(void)
{
	vkDestroyDevice(m_Handle, nullptr);
}

const VkPhysicalDevice& Device::GetPhysicalDevice(void) const noexcept
{
	return m_PhysicalDevice;
}

VkCommandPool const& Device::GetCommandPool(void) const noexcept
{
	return m_CommandPool;
}

bool Device::ExtensionSupported(const std::string& name) const noexcept
{
	return m_SupportedExtensions.end() != std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), name);
}

void Device::SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue) const noexcept
{
	VkSubmitInfo submitInfo = vks::inits::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VkFenceCreateInfo fenceInfo = vks::inits::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHK(vkCreateFence(m_Handle, &fenceInfo, nullptr, &fence));
	VK_CHK(vkQueueSubmit(queue, 1, &submitInfo, fence));
	VK_CHK(vkWaitForFences(m_Handle, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
	vkDestroyFence(m_Handle, fence, nullptr);
}

std::optional<VkFormat> Device::SupportedDepthStencilFormat(void) const noexcept
{
	std::array<VkFormat, 3> formatList = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
	};

	for (auto& format : formatList)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}

	return std::nullopt;
}

/**
* Get the index of a queue family that best satisfies the requested queue Flags
*
* @param queueFlags reqeusted queue flags
*
* @return optional index of a queue index
*/
std::optional<uint32_t> Device::GetQueueFamilyIndex(VkQueueFlags queueFlags) const noexcept
{
	/* when requesting dedicated COMPUTE or TRANSFER queue,
	   ensure the resulting queue does not have GRAPHICS capability
	*/
	if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags || (queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
	{
		for (auto it = m_QueueFamilyProperties.begin(); it != m_QueueFamilyProperties.end(); it++)
		{
			if (it->queueFlags & queueFlags & ~VK_QUEUE_GRAPHICS_BIT)
			{
				return std::distance(m_QueueFamilyProperties.begin(), it);
			}
		}
	}

	for (auto it = m_QueueFamilyProperties.begin(); it != m_QueueFamilyProperties.end(); it++)
	{
		if ((it->queueFlags & queueFlags) == queueFlags)
		{
			return std::distance(m_QueueFamilyProperties.begin(), it);
		}
	}

	return std::nullopt;
}

/**
* Get the index of a memory type that has all the requested property bits set
*
* @param memoryTypeBits Bit mask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
* @param properties Bit mask of properties for the memory type to request
*
* @return optional index of the requested memory type
*/
std::optional<uint32_t> Device::GetMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties) const noexcept
{
	for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++)
	{
		if (memoryTypeBits & 1 << i)
		{
			if ((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
	}

	return std::nullopt;
}
}