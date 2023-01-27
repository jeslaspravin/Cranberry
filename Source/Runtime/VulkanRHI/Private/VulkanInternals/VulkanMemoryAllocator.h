/*!
 * \file VulkanMemoryAllocator.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Memory/SmartPointers.h"

#include <vulkan_core.h>

class VulkanDevice;
struct VulkanMemoryBlock;
struct VulkanMemoryAllocation;

class IVulkanMemoryAllocator
{
protected:
    VulkanDevice *device;
    IVulkanMemoryAllocator(VulkanDevice *vDevice);

public:
    virtual ~IVulkanMemoryAllocator() = default;

    static SharedPtr<IVulkanMemoryAllocator> createAllocator(VulkanDevice *vDevice);

    virtual void initAllocator() = 0;
    virtual void destroyAllocator() = 0;

    virtual VulkanMemoryAllocation allocateBuffer(VkBuffer buffer, bool cpuAccessible) = 0;
    virtual VulkanMemoryAllocation allocateImage(VkImage image, bool cpuAccessible, bool bIsOptimalTiled) = 0;

    virtual void deallocateBuffer(VkBuffer buffer, const VulkanMemoryAllocation &allocation) = 0;
    virtual void deallocateImage(VkImage image, const VulkanMemoryAllocation &allocation, bool bIsOptimalTiled) = 0;

    virtual void mapBuffer(VulkanMemoryAllocation &allocation) = 0;
    virtual void unmapBuffer(VulkanMemoryAllocation &allocation) = 0;
    virtual void mapImage(VulkanMemoryAllocation &block) = 0;
    virtual void unmapImage(VulkanMemoryAllocation &block) = 0;
};

namespace std
{
template <>
struct default_delete<IVulkanMemoryAllocator>
{
    void operator() (IVulkanMemoryAllocator *_Ptr) const noexcept
    {
        _Ptr->destroyAllocator();
        delete _Ptr;
    }
};
} // namespace std