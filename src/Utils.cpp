#include <string>
#include <fstream>

#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>

#include "vks/Utils.hpp"

namespace vks
{
namespace utils
{
std::string statusString(VkResult result) noexcept
{

    switch (result)
    {
#define STR(r) case (r): return #r
        STR(VK_SUCCESS);
        STR(VK_NOT_READY);
        STR(VK_TIMEOUT);
        STR(VK_EVENT_SET);
        STR(VK_EVENT_RESET);
        STR(VK_INCOMPLETE);
        STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        STR(VK_ERROR_INITIALIZATION_FAILED);
        STR(VK_ERROR_DEVICE_LOST);
        STR(VK_ERROR_MEMORY_MAP_FAILED);
        STR(VK_ERROR_LAYER_NOT_PRESENT);
        STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        STR(VK_ERROR_FEATURE_NOT_PRESENT);
        STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        STR(VK_ERROR_TOO_MANY_OBJECTS);
        STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        STR(VK_ERROR_FRAGMENTED_POOL);
        STR(VK_ERROR_UNKNOWN);
        STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
        STR(VK_ERROR_FRAGMENTATION);
        STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
        STR(VK_PIPELINE_COMPILE_REQUIRED);
        STR(VK_ERROR_SURFACE_LOST_KHR);
        STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(VK_SUBOPTIMAL_KHR);
        STR(VK_ERROR_OUT_OF_DATE_KHR);
        STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(VK_ERROR_VALIDATION_FAILED_EXT);
        STR(VK_ERROR_INVALID_SHADER_NV);
        STR(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
        STR(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
        STR(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
        STR(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
        STR(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
        STR(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);
        STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
        STR(VK_ERROR_NOT_PERMITTED_KHR);
        STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
        STR(VK_THREAD_IDLE_KHR);
        STR(VK_THREAD_DONE_KHR);
        STR(VK_OPERATION_DEFERRED_KHR);
        STR(VK_OPERATION_NOT_DEFERRED_KHR);
        STR(VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR);
        STR(VK_ERROR_COMPRESSION_EXHAUSTED_EXT);
        STR(VK_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
    default:
        return "VK_UNKNOWN_ERROR";
    }
}

void exitFatal(const std::string& message, int32_t exitCode)
{
    spdlog::critical(message);
    exit(exitCode);
}

void exitFatal(const std::string& message, VkResult resultCode)
{
    exitFatal(message, static_cast<int32_t>(resultCode));
}

VkShaderModule loadShader(const char* fileName, VkDevice device)
{
    std::ifstream ifs(fileName, std::ios::binary);

    if (!ifs.is_open())
    {
        spdlog::error("Could not open shader file \"{}\"", fileName);
        ifs.close();
        return VK_NULL_HANDLE;
    }

    std::vector<uint8_t> shaderCode((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    ifs.close();

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = shaderCode.size();
    moduleCreateInfo.pCode = (uint32_t*)shaderCode.data();

    VK_CHK(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

    return shaderModule;
}
}
}
