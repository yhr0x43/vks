#include "vks/Framebuffer.hpp"
#include "vks/Utils.hpp"

namespace vks
{
void FramebufferAttachment::Init(void)
{
    VK_CHK(vkCreateImage(m_Device, &m_ImageCreateInfo, nullptr, &m_Image));
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(m_Device, m_Image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = m_Device.GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).value();
    VK_CHK(vkAllocateMemory(m_Device, &memAllloc, nullptr, &m_Memory));
    VK_CHK(vkBindImageMemory(m_Device, m_Image, m_Memory, 0));

    m_ImageViewCreateInfo.image = m_Image;
    VK_CHK(vkCreateImageView(m_Device, &m_ImageViewCreateInfo, nullptr, &m_ImageView));
}

void FramebufferAttachment::Destroy(void)
{
    vkDestroyImageView(m_Device, m_ImageView, nullptr);
    vkFreeMemory(m_Device, m_Memory, nullptr);
    vkDestroyImage(m_Device, m_Image, nullptr);
}

/**
* Default constructor
* 
* @param device a valid reference to vks::Device
* @param imageCI completed VkImageCreateInfo struct
* @param imageViewCI a VkImageViewCreateInfo struct, its `image` field will be filled by the constructor
*/
FramebufferAttachment::FramebufferAttachment(
    Device const& device,
    VkImageCreateInfo const& imageCI,
    VkImageViewCreateInfo& imageViewCI
) noexcept
    : m_Device(device), m_ImageCreateInfo(imageCI), m_ImageViewCreateInfo(imageViewCI)
{
    Init();
}

FramebufferAttachment::~FramebufferAttachment(void) noexcept
{
    Destroy();
}

void FramebufferAttachment::Recreate(VkExtent3D extent) noexcept
{
    Destroy();
    m_ImageCreateInfo.extent = extent;
    Init();
}

VkImageView const& FramebufferAttachment::GetView(void) const noexcept
{
    return m_ImageView;
}
}