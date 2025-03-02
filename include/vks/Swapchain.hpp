#pragma once

#include <vulkan/vulkan.h>

#include "Framebuffer.hpp"
#include "vks/Instance.hpp"
#include "vks/Device.hpp"
#include "vks/VulkanEncapsulate.hpp"

namespace vks
{
class Swapchain : public VulkanEncapsulate<VkSwapchainKHR>
{
    Instance const& m_Instance;
    Device const& m_Device;
    VkSurfaceKHR m_Surface;

    uint32_t m_QueueIndex;
    VkFormat m_ColorFormat;
    VkColorSpaceKHR m_ColorSpace;

    uint32_t m_ImageIndex;
    uint32_t m_CurrentFrame;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_Views;

    vks::FramebufferAttachment* m_pDepthStencil;
    std::vector<VkFramebuffer> m_Framebuffers;

    std::vector<VkSemaphore> m_PresentDoneSemaphore;
    std::vector<VkSemaphore> m_RenderDoneSemaphore;
    std::vector<VkFence> m_WaitFences;

    VkCommandPool m_CmdPool;
    std::vector<VkCommandBuffer> m_CmdBuffers;

    VkRenderPass m_RenderPass;

public:
    void Recreate(uint32_t& width, uint32_t& height, bool vsync) noexcept;
    VkResult AcquireNextImage(void) noexcept;
    VkResult QueueSubmit(VkQueue queue) const noexcept;
    VkResult QueuePresent(VkQueue queue) const noexcept;

    VkRenderPass const& GetRenderPass(void) const noexcept;
    uint32_t const& GetQueueIndex(void) const noexcept;
    uint32_t const& GetCurrentFrame(void) const noexcept;
    size_t GetImageCount(void) const noexcept;
    VkFormat const& GetColorFormat(void) const noexcept;
    std::vector<VkImageView> const& GetImageViews(void) const noexcept;

    VkCommandBuffer const& GetCommandBuffer(void) const noexcept;
    VkFramebuffer const& GetFramebuffer(void) const noexcept;

    Swapchain(
        Instance const& instance,
        Device const& device,
        VkSurfaceKHR surface,
        uint32_t& width,
        uint32_t& height,
        bool vsync
        );
    ~Swapchain(void) noexcept;
};
}
