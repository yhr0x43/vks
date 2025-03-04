#include <spdlog/spdlog.h>

#include <vulkan/vulkan.h>

#include "vks/Debug.hpp"
#include "vks/Inits.hpp"
#include "vks/Utils.hpp"
#include "vks/Instance.hpp"

namespace vks
{
Instance::Instance(
    VkApplicationInfo appInfo,
    bool validation,
    std::vector<const char*> enabledExtensions
) noexcept
{
    std::vector<const char*> exts(enabledExtensions);

    exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if WIN32
    exts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    // Get extensions supported by the instance and store for later use
    uint32_t extCnt = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCnt, nullptr);
    if (extCnt > 0) {
        std::vector<VkExtensionProperties> props(extCnt);
        VK_CHK(vkEnumerateInstanceExtensionProperties(nullptr, &extCnt, &props.front()));
        spdlog::debug("Supported Vulkan instance extensions:");
        for (VkExtensionProperties& prop : props) {
            spdlog::debug("{}", prop.extensionName);
            m_SupportedExtensions.push_back(prop.extensionName);
        }
    }

    // Enabled requested instance extensions
    for (const char* ext : exts) {
        std::string str(ext);
        if (!ExtensionSupported(ext)) {
            spdlog::error("Enabled Vulkan instance extension {} is not present", ext);
        }
    }

    VkInstanceCreateInfo instCreateInfo = vks::inits::instanceCreateInfo({});
    instCreateInfo.pApplicationInfo = &appInfo;

    if (ExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        /* VkDebugUtilsMessengerCreateInfoEXT is required when VK_EXT_debug_utils is a requested extension */
        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
        vks::debug::setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
        debugUtilsMessengerCI.pNext = instCreateInfo.pNext;
        instCreateInfo.pNext = &debugUtilsMessengerCI;
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (!exts.empty()) {
        instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
        instCreateInfo.ppEnabledExtensionNames = exts.data();
    }

    /* The VK_LAYER_KHRONOS_validation contains all current validation functionality */
    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (validation) {
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
        bool validationLayerPresent = false;
        for (VkLayerProperties& layer : instanceLayerProperties) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                validationLayerPresent = true;
                break;
            }
        }
        if (validationLayerPresent) {
            instCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instCreateInfo.enabledLayerCount = 1;
        }
        else {
            spdlog::info("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
        }
    }
    VK_CHK(vkCreateInstance(&instCreateInfo, nullptr, &m_Handle));
}

Instance::~Instance(void)
{
    if (VK_NULL_HANDLE != m_Handle)
    {
        vkDestroyInstance(m_Handle, nullptr);
    }
}

bool Instance::ExtensionSupported(const std::string& name)
{
    return m_SupportedExtensions.end() != std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), name);
}
}
