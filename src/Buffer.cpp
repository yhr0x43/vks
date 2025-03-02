#include "vks/Inits.hpp"
#include "vks/Utils.hpp"

#include "vks/Buffer.hpp"

namespace vks
{
Buffer::Buffer(
	Device const& device,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags memoryPropertyFlags,
	VkDeviceSize size,
	void* data
) noexcept
	: m_Device(device), m_Size(size), m_pMapped(nullptr)
{
	VkBufferCreateInfo bufferCreateInfo = vks::inits::bufferCreateInfo(0, usageFlags, size);
	VK_CHK(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &m_Handle));

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, m_Handle, &memReqs);

	VkMemoryAllocateInfo memAlloc = vks::inits::memoryAllocateInfo();
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = device.GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags).value();

	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memAlloc.pNext = &allocFlagsInfo;
	}
	VK_CHK(vkAllocateMemory(device, &memAlloc, nullptr, &m_Memory));

	if (data != nullptr)
	{
		VK_CHK(Map(size));
		memcpy(m_pMapped, data, size);
		/* manual flush when COHERENT bit is not requested */
		if (!(memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		{
			VK_CHK(Flush(size));
		}
		Unmap();
	}

	VK_CHK(vkBindBufferMemory(device, m_Handle, m_Memory, 0));
}

Buffer::~Buffer(void) noexcept
{
	if (m_Handle)
	{
		vkDestroyBuffer(m_Device, m_Handle, nullptr);
	}
	if (m_Memory)
	{
		vkFreeMemory(m_Device, m_Memory, nullptr);
	}
}

VkResult Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) const noexcept
{
	VkMappedMemoryRange mappedRange = vks::inits::mappedMemoryRange();
	mappedRange.memory = m_Memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(m_Device, 1, &mappedRange);
}

VkResult Buffer::Map(VkDeviceSize size, VkDeviceSize offset, VkMemoryMapFlags flags) noexcept
{
	return vkMapMemory(m_Device, m_Memory, offset, size, flags, &m_pMapped);
}

void Buffer::Unmap(void) noexcept
{
	if (m_pMapped)
	{
		vkUnmapMemory(m_Device, m_Memory);
		m_pMapped = nullptr;
	}
}

void Buffer::CopyData(const void* data, size_t size) noexcept
{
	assert(m_pMapped);
	memcpy(m_pMapped, data, size);
}

void Buffer::CopyFrom(vks::Buffer& src, std::optional<VkBufferCopy> bufferCopy) const noexcept
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vks::inits::commandBufferAllocateInfo(m_Device.GetCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkCommandBuffer copyCmd;
	VK_CHK(vkAllocateCommandBuffers(m_Device, &cmdBufAllocateInfo, &copyCmd));

	VkQueue queue;
	vkGetDeviceQueue(m_Device, m_Device.QueueIndex.Graphics, 0, &queue);

	VkCommandBufferBeginInfo cmdBufInfo = vks::inits::commandBufferBeginInfo();
	VK_CHK(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

	VkBufferCopy tmpBufferCopy = bufferCopy.value_or(VkBufferCopy{ .size = src.m_Size });
	vkCmdCopyBuffer(copyCmd, src.m_Handle, m_Handle, 1, &tmpBufferCopy);

	VK_CHK(vkEndCommandBuffer(copyCmd));

	m_Device.SubmitCommandBuffer(copyCmd, queue);
}
}