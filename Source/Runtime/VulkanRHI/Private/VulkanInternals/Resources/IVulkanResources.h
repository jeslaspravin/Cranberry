/*!
 * \file IVulkanResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"

#include <vulkan_core.h>

class String;
struct VulkanMemoryBlock;

class IVulkanResources
{

public:
    virtual VkObjectType getObjectType() const = 0;
    virtual const String &getObjectTypeName() const = 0;
    virtual String getObjectName() const = 0;
    virtual uint64 getDispatchableHandle() const { return 0; }
};

struct VulkanMemoryAllocation
{
    uint64 byteOffset;
    uint64 byteSize;
    const VulkanMemoryBlock *memBlock = nullptr;
    VkDeviceMemory deviceMemory;
    void *mappedMemory = nullptr;
};

class IVulkanMemoryResources : public IVulkanResources
{
private:
    VulkanMemoryAllocation memAllocation;

public:
    virtual uint64 requiredSize() const = 0;
    virtual bool canAllocateMemory() const = 0;
    uint64 allocatedSize() const;
    uint64 allocationOffset() const;
    VkDeviceMemory getDeviceMemory() const;
    void *getMappedMemory() const;

    // Internal use only
    void setMemoryData(VulkanMemoryAllocation allocation);
    const VulkanMemoryAllocation &getMemoryData() const;
    VulkanMemoryAllocation &getMemoryData();
};
