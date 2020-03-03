#pragma once

#include <vulkan_core.h>
#include "../../Core/Platform/PlatformTypes.h"

class String;
struct VulkanMemoryBlock;

class IVulkanResources
{

public:

    virtual VkObjectType getObjectType() const = 0;
    virtual const String& getObjectTypeName() const = 0;
    virtual String getObjectName() const = 0;
    virtual void setObjectName(const String& name) {};
    virtual uint64 getDispatchableHandle() const { return 0; }
};

class IVulkanMemoryResources : public IVulkanResources
{
private:
    VulkanMemoryBlock* blockData;
public:
    virtual uint64 requiredSize() = 0;
    virtual bool canAllocateMemory() = 0;
    uint64 allocatedSize();
    uint64 allocationOffset();
    VkDeviceMemory getDeviceMemory();

    // Internal use only
    void setMemoryData(VulkanMemoryBlock* block);
    VulkanMemoryBlock* getMemoryData();
};
