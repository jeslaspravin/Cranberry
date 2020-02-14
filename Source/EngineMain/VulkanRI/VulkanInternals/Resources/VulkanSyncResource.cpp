#include "VulkanSyncResource.h"
#include "../VulkanDevice.h"

#include <assert.h>
#include "../../../Core/Logger/Logger.h"
#include "../../VulkanGraphicsHelper.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanSemaphore,VK_OBJECT_TYPE_SEMAPHORE)

VulkanSemaphore::VulkanSemaphore(const VulkanDevice* deviceInstance)
	:BaseType(),ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance)),vulkanDevice(deviceInstance)
{}

void VulkanSemaphore::waitForSignal() const
{
	Logger::warn("VulkanSemaphore", "%s() : Cannot wait on binary semaphores from host", __func__);
}

bool VulkanSemaphore::isSignaled() const
{
	Logger::warn("VulkanSemaphore", "%s() : Cannot check state on binary semaphores from host", __func__);
	return false;
}

void VulkanSemaphore::resetSignal()
{
	Logger::warn("VulkanSemaphore", "%s() : Cannot reset state on binary semaphores from host", __func__);
}

void VulkanSemaphore::init()
{
	reinitResources();
}

void VulkanSemaphore::reinitResources()
{
	assert(ownerDevice && vulkanDevice);
	VkSemaphore nextSemaphore;

	CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
	if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore) == VK_SUCCESS)
	{
		release();
		semaphore = nextSemaphore;
	}
	else
	{
		Logger::error("VulkanSemaphore", "%s() : Reinit failed to create new semaphore", __func__);
	}
}

void VulkanSemaphore::release()
{
	assert(ownerDevice && vulkanDevice);
	if (semaphore)
	{
		vulkanDevice->vkDestroySemaphore(ownerDevice, semaphore, nullptr);
	}
}

String VulkanSemaphore::getObjectName() const
{
	return resourceName;
}

void VulkanSemaphore::setObjectName(const String& name)
{
    resourceName = name;
}

uint64 VulkanSemaphore::getDispatchableHandle() const
{
	return (uint64)semaphore;
}

//////////////////////////////////////////////////////////////////////////
// VulkanTimelineSemaphore 

DEFINE_VK_GRAPHICS_RESOURCE(VulkanTimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE)

VulkanTimelineSemaphore::VulkanTimelineSemaphore(const VulkanDevice* deviceInstance)
	:BaseType(), ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance)), vulkanDevice(deviceInstance)
{}

void VulkanTimelineSemaphore::waitForSignal(uint64 value) const
{
	if (!isSignaled(value))
	{
		SEMAPHORE_WAIT_INFO(waitInfo);
		waitInfo.pSemaphores = &semaphore;
		waitInfo.semaphoreCount = 1;
		waitInfo.pValues = &value;
		vulkanDevice->vkWaitSemaphoresKHR(ownerDevice, &waitInfo, 2000000000/*2 Seconds*/);
	}
}

bool VulkanTimelineSemaphore::isSignaled(uint64 value) const
{
	return value >= currentValue();
}

void VulkanTimelineSemaphore::resetSignal(uint64 value)
{
	uint64 currentVal = currentValue();
	if (value > currentVal && (value - currentVal) < vulkanDevice->maxAllowedTimelineOffset())
	{
		SEMAPHORE_SIGNAL_INFO(signalInfo);
		signalInfo.semaphore = semaphore;
		signalInfo.value = value;
		if (vulkanDevice->vkSignalSemaphoreKHR(ownerDevice, &signalInfo) != VK_SUCCESS)
		{
			Logger::error("VulkanTimelineSemaphore", "%s() : Signaling to value %d failed", __func__, value);
		}
	}
}

uint64 VulkanTimelineSemaphore::currentValue() const
{
	uint64 counter;
	vulkanDevice->vkGetSemaphoreCounterValueKHR(ownerDevice, semaphore, &counter);
	return counter;
}

String VulkanTimelineSemaphore::getObjectName() const
{
    return resourceName;
}

void VulkanTimelineSemaphore::setObjectName(const String& name)
{
    resourceName = name;
}

uint64 VulkanTimelineSemaphore::getDispatchableHandle() const
{
	return (uint64)semaphore;
}

void VulkanTimelineSemaphore::init()
{
	reinitResources();
}

void VulkanTimelineSemaphore::reinitResources()
{
	assert(ownerDevice && vulkanDevice);
	VkSemaphore nextSemaphore;

	CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
	CREATE_TYPED_SEMAPHORE_INFO(typedSemaphoreCreateInfo);
	semaphoreCreateInfo.pNext = &typedSemaphoreCreateInfo;

	if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore) == VK_SUCCESS)
	{
		release();
		semaphore = nextSemaphore;
	}
	else
	{
		Logger::error("VulkanSemaphore", "%s() : Reinit failed to create new semaphore", __func__);
	}
}

void VulkanTimelineSemaphore::release()
{
	assert(ownerDevice && vulkanDevice);
	if (semaphore)
	{
		vulkanDevice->vkDestroySemaphore(ownerDevice, semaphore, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////
// VulkanFence

DEFINE_VK_GRAPHICS_RESOURCE(VulkanFence, VK_OBJECT_TYPE_FENCE)

VulkanFence::VulkanFence(const VulkanDevice* deviceInstance)
	:BaseType(), ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance)), vulkanDevice(deviceInstance)
{}

void VulkanFence::waitForSignal() const
{
	vulkanDevice->vkWaitForFences(ownerDevice, 1, &fence,VK_TRUE, 2000000000/*2 Seconds*/);
}

bool VulkanFence::isSignaled() const
{
	return vulkanDevice->vkGetFenceStatus(ownerDevice, fence) == VK_SUCCESS;
}

void VulkanFence::resetSignal()
{
	vulkanDevice->vkResetFences(ownerDevice, 1, &fence);
}

void VulkanFence::init()
{
	reinitResources();
}

void VulkanFence::reinitResources()
{
	assert(ownerDevice && vulkanDevice);
	VkFence nextFence;

	CREATE_FENCE_INFO(fenceCreateInfo);
	if (vulkanDevice->vkCreateFence(ownerDevice, &fenceCreateInfo, nullptr, &nextFence) == VK_SUCCESS)
	{
		release();
		fence = nextFence;
	}
	else
	{
		Logger::error("VulkanFence", "%s() : Failed recreating fence", __func__);
	}
}

void VulkanFence::release()
{
	if (fence && fence != VK_NULL_HANDLE)
	{
		vulkanDevice->vkDestroyFence(ownerDevice, fence, nullptr);
		fence = nullptr;
	}
}

String VulkanFence::getObjectName() const
{
    return resourceName;
}

void VulkanFence::setObjectName(const String& name)
{
	resourceName = name;
}

uint64 VulkanFence::getDispatchableHandle() const
{
	return (uint64)fence;
}
