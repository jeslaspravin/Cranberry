#pragma once

#include <vulkan_core.h>
#include "../../Core/Platform/PlatformTypes.h"

class String;

class IVulkanResources
{

public:

    virtual VkObjectType getObjectType() const = 0;
    virtual const String& getObjectTypeName() const = 0;
    virtual String getObjectName() const = 0;
    virtual void setObjectName(const String& name) {};
    virtual uint64 getDispatchableHandle() const { return 0; }
};