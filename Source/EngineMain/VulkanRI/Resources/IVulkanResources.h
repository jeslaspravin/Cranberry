#pragma once

#include <vulkan_core.h>

class String;

class IVulkanResources
{

public:

	virtual VkObjectType getObjectType() const = 0;
	virtual const String& getObjectTypeName() const = 0;
};