#pragma once

#include "Types/CoreTypes.h"
#include "EngineRendererExports.h"

#include <vulkan_core.h>

class String;
struct VulkanMemoryBlock;

class IVulkanResources
{

public:

    virtual VkObjectType getObjectType() const = 0;
    virtual const String& getObjectTypeName() const = 0;
    virtual String getObjectName() const = 0;
    virtual uint64 getDispatchableHandle() const { return 0; }
};

class IVulkanMemoryResources : public IVulkanResources
{
private:
    VulkanMemoryBlock* blockData;
public:
    virtual uint64 requiredSize() const = 0;
    virtual bool canAllocateMemory() const = 0;
    uint64 allocatedSize() const;
    uint64 allocationOffset() const;
    VkDeviceMemory getDeviceMemory() const;
    void* getMappedMemory() const;

    // Internal use only
    void setMemoryData(VulkanMemoryBlock* block);
    VulkanMemoryBlock* getMemoryData() const;
};
