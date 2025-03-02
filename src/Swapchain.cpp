#include <array>

#include "vks/Inits.hpp"
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
)
    : m_Instance(instance), m_Device(device), m_Surface(surface)
{
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

    VkFormat depthFormat = device.SupportedDepthStencilFormat().value();

    VkImageCreateInfo imageCI =
        vks::inits::imageCreateInfo(
            (VkImageCreateFlags)0,
            VK_IMAGE_TYPE_2D,
            depthFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
    imageCI.extent = { width, height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;

    VkImageViewCreateInfo imageViewCI =
        vks::inits::imageViewCreateInfo(VK_NULL_HANDLE, VK_IMAGE_VIEW_TYPE_2D, depthFormat);
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT)
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    m_pDepthStencil = new vks::FramebufferAttachment(device, imageCI, imageViewCI);

    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = m_ColorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format = m_Device.SupportedDepthStencilFormat().value();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));

    VkCommandPoolCreateInfo cmdPoolInfo = vks::inits::commandPoolCreateInfo(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    cmdPoolInfo.queueFamilyIndex = m_QueueIndex;
    VK_CHK(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &m_CmdPool));

    Recreate(width, height, vsync);
}

Swapchain::~Swapchain(void) noexcept
{
    delete m_pDepthStencil;

    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

    for (uint32_t i = 0; i < m_Images.size(); i++)
    {
        vkDestroyImageView(m_Device, m_Views[i], nullptr);
        vkDestroySemaphore(m_Device, m_PresentDoneSemaphore[i], nullptr);
        vkDestroySemaphore(m_Device, m_RenderDoneSemaphore[i], nullptr);
        vkDestroyFence(m_Device, m_WaitFences[i], nullptr);
        vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr);
    }

    vkFreeCommandBuffers(m_Device, m_CmdPool, (uint32_t)m_CmdBuffers.size(), m_CmdBuffers.data());
    vkDestroyCommandPool(m_Device, m_CmdPool, nullptr);

    vkDestroySwapchainKHR(m_Device, m_Handle, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}

void Swapchain::Recreate(uint32_t& width, uint32_t& height, bool vsync) noexcept
{
    m_pDepthStencil->Recreate({ width, height, 1 });

    VkSurfaceCapabilitiesKHR surfCaps;
    VK_CHK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), m_Surface, &surfCaps));

    VkExtent2D swapchainExtent = {};
    // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
    if (surfCaps.currentExtent.width == (uint32_t)-1)
    {
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

    uint32_t presentModeCount;
    VK_CHK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Surface, &presentModeCount, NULL));
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data()));

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

    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
    {
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }
    spdlog::debug("Desired number of swapchain images: {}", desiredNumberOfSwapchainImages);

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
    swapchainCI.oldSwapchain = m_Handle;
    //swapchainCI.clipped = VK_TRUE;
    swapchainCI.clipped = VK_FALSE;
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
    VK_CHK(vkGetSwapchainImagesKHR(m_Device, m_Handle, &imageCnt, nullptr));
    spdlog::debug("Swapchain image count: {}", imageCnt);

    m_CurrentFrame = 0;

    m_Images.resize(imageCnt);
    VK_CHK(vkGetSwapchainImagesKHR(m_Device, m_Handle, &imageCnt, m_Images.data()));

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

    m_PresentDoneSemaphore.resize(imageCnt);
    m_RenderDoneSemaphore.resize(imageCnt);
    m_WaitFences.resize(imageCnt);

    VkSemaphoreCreateInfo semaphoreCreateInfo = vks::inits::semaphoreCreateInfo();
    VkFenceCreateInfo fenceCreateInfo = vks::inits::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    for (size_t i = 0; i < imageCnt; i++) {
        VK_CHK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_PresentDoneSemaphore[i]));
        VK_CHK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderDoneSemaphore[i]));
        VK_CHK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_WaitFences[i]));
    }

    VkImageView attachments[2];
    attachments[1] = m_pDepthStencil->GetView();
    VkFramebufferCreateInfo framebufferInfo = vks::inits::framebufferCreateInfo();
    framebufferInfo.renderPass = m_RenderPass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    m_Framebuffers.resize(imageCnt);
    for (uint32_t i = 0; i < imageCnt; i++) {
        attachments[0] = m_Views[i];
        VK_CHK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]));
    }

    /* Vulkan specifies bufferCount must be greater than 0, which is not the case for first initialization */
    if (0 != m_CmdBuffers.size()) {
        vkFreeCommandBuffers(m_Device, m_CmdPool, (uint32_t)m_CmdBuffers.size(), m_CmdBuffers.data());
    }

    m_CmdBuffers.resize(imageCnt);
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        vks::inits::commandBufferAllocateInfo(m_CmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)m_CmdBuffers.size());
    VK_CHK(vkAllocateCommandBuffers(m_Device, &cmdBufAllocateInfo, m_CmdBuffers.data()));
}

VkResult Swapchain::AcquireNextImage(void) noexcept
{
    m_CurrentFrame = (m_CurrentFrame + 1) % GetImageCount();
    //spdlog::trace("Acquiring in-flight frame: {}", m_CurrentFrame);

    vkWaitForFences(m_Device, 1, &m_WaitFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device, 1, &m_WaitFences[m_CurrentFrame]);

    VkResult result =
        vkAcquireNextImageKHR(m_Device, m_Handle, UINT64_MAX, m_PresentDoneSemaphore[m_CurrentFrame], (VkFence)VK_NULL_HANDLE, &m_ImageIndex);
    return result;
}

VkResult Swapchain::QueueSubmit(VkQueue queue) const noexcept
{
    VkSubmitInfo submitInfo = vks::inits::submitInfo();
    VkSemaphore waitSemaphores[] = { m_PresentDoneSemaphore[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CmdBuffers[m_CurrentFrame];
    VkSemaphore signalSemaphores[] = { m_RenderDoneSemaphore[m_CurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    //spdlog::trace("Queue submit wait-fence status: {}", vks::utils::statusString(vkGetFenceStatus(m_Device, m_WaitFences[m_CurrentFrame])));
    return vkQueueSubmit(queue, 1, &submitInfo, m_WaitFences[m_CurrentFrame]);
}

VkResult Swapchain::QueuePresent(VkQueue queue) const noexcept
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Handle;
    presentInfo.pImageIndices = &m_ImageIndex;
    presentInfo.pWaitSemaphores = &m_RenderDoneSemaphore[m_CurrentFrame];
    presentInfo.waitSemaphoreCount = 1;
    return vkQueuePresentKHR(queue, &presentInfo);
}

VkRenderPass const& Swapchain::GetRenderPass(void) const noexcept
{
    return m_RenderPass;
}

uint32_t const& Swapchain::GetQueueIndex(void) const noexcept
{
    return m_QueueIndex;
}

uint32_t const& Swapchain::GetCurrentFrame(void) const noexcept
{
    return m_CurrentFrame;
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

VkCommandBuffer const& Swapchain::GetCommandBuffer(void) const noexcept
{
    return m_CmdBuffers[m_CurrentFrame];
}

VkFramebuffer const& Swapchain::GetFramebuffer(void) const noexcept
{
    return m_Framebuffers[m_ImageIndex];
}
}
