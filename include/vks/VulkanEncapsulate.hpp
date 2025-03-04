#pragma once

#include <vulkan/vulkan.h>

namespace vks
{
class NonCopyable
{
protected:
    NonCopyable(void) = default;
    ~NonCopyable(void) = default;

    NonCopyable(NonCopyable const&) = delete;
    void operator=(NonCopyable const&) = delete;
};

template<class T>
class VulkanEncapsulate : public NonCopyable
{
protected:
    T m_Handle{ VK_NULL_HANDLE };

public:
    operator T() const
    {
        return m_Handle;
    };
};
}
