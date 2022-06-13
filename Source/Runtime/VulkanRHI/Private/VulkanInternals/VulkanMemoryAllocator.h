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

    virtual VulkanMemoryBlock *allocateBuffer(VkBuffer buffer, bool cpuAccessible) = 0;
    virtual VulkanMemoryBlock *allocateImage(VkImage image, bool cpuAccessible, bool bIsOptimalTiled) = 0;

    virtual void deallocateBuffer(VkBuffer buffer, VulkanMemoryBlock *block) = 0;
    virtual void deallocateImage(VkImage image, VulkanMemoryBlock *block, bool bIsOptimalTiled) = 0;

    virtual void mapBuffer(VulkanMemoryBlock *block) = 0;
    virtual void unmapBuffer(VulkanMemoryBlock *block) = 0;
    virtual void mapImage(VulkanMemoryBlock *block) = 0;
    virtual void unmapImage(VulkanMemoryBlock *block) = 0;
};

namespace std
{
template <>
struct default_delete<IVulkanMemoryAllocator>
{
    void operator()(IVulkanMemoryAllocator *_Ptr) const noexcept
    {
        _Ptr->destroyAllocator();
        delete _Ptr;
    }
};
} // namespace std