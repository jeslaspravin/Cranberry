/*!
 * \file VulkanSyncResource.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/Resources/VulkanSyncResource.h"
#include "Logger/Logger.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/VulkanDevice.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanSemaphore, VK_OBJECT_TYPE_SEMAPHORE)

VulkanSemaphore::VulkanSemaphore(const VulkanDevice *deviceInstance)
    : BaseType()
    , ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance))
    , vulkanDevice(deviceInstance)
    , semaphore(nullptr)
{}

void VulkanSemaphore::waitForSignal() const { LOG_WARN("VulkanSemaphore", "Cannot wait on binary semaphores from host"); }

bool VulkanSemaphore::isSignaled() const
{
    LOG_WARN("VulkanSemaphore", "Cannot check state on binary semaphores from host");
    return false;
}

void VulkanSemaphore::resetSignal() { LOG_WARN("VulkanSemaphore", "Cannot reset state on binary semaphores from host"); }

void VulkanSemaphore::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanSemaphore::reinitResources()
{
    release();
    BaseType::reinitResources();
    fatalAssertf(ownerDevice && vulkanDevice, "Required devices cannot be null");
    VkSemaphore nextSemaphore;

    CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
    if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore) == VK_SUCCESS)
    {
        semaphore = nextSemaphore;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        LOG_ERROR("VulkanSemaphore", "Reinit failed to create new semaphore");
    }
}

void VulkanSemaphore::release()
{
    fatalAssertf(ownerDevice && vulkanDevice, "Required devices cannot be null");
    if (semaphore)
    {
        vulkanDevice->vkDestroySemaphore(ownerDevice, semaphore, nullptr);
        semaphore = nullptr;
    }
    BaseType::release();
}

String VulkanSemaphore::getObjectName() const { return getResourceName(); }

uint64 VulkanSemaphore::getDispatchableHandle() const { return (uint64)semaphore; }

//////////////////////////////////////////////////////////////////////////
// VulkanTimelineSemaphore
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanTimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE)

VulkanTimelineSemaphore::VulkanTimelineSemaphore(const VulkanDevice *deviceInstance)
    : BaseType()
    , ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance))
    , vulkanDevice(deviceInstance)
    , semaphore(nullptr)
{}

void VulkanTimelineSemaphore::waitForSignal(uint64 value) const
{
    if (!isSignaled(value) && GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get())
    {
        SEMAPHORE_WAIT_INFO(waitInfo);
        waitInfo.pSemaphores = &semaphore;
        waitInfo.semaphoreCount = 1;
        waitInfo.pValues = &value;
        vulkanDevice->vkWaitSemaphores(ownerDevice, &waitInfo, GlobalRenderVariables::MAX_SYNC_RES_WAIT_TIME.get());
    }
}

bool VulkanTimelineSemaphore::isSignaled(uint64 value) const { return value >= currentValue(); }

void VulkanTimelineSemaphore::resetSignal(uint64 value)
{
    uint64 currentVal = currentValue();
    if (GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get() && value > currentVal
        && (value - currentVal) < GlobalRenderVariables::MAX_TIMELINE_OFFSET.get())
    {
        SEMAPHORE_SIGNAL_INFO(signalInfo);
        signalInfo.semaphore = semaphore;
        signalInfo.value = value;

        if (vulkanDevice->vkSignalSemaphore(ownerDevice, &signalInfo) != VK_SUCCESS)
        {
            LOG_ERROR("VulkanTimelineSemaphore", "Signaling to value {} failed", value);
        }
    }
}

uint64 VulkanTimelineSemaphore::currentValue() const
{
    uint64 counter = 0;
    if (GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get())
    {
        vulkanDevice->vkGetSemaphoreCounterValue(ownerDevice, semaphore, &counter);
    }
    return counter;
}

String VulkanTimelineSemaphore::getObjectName() const { return getResourceName(); }

uint64 VulkanTimelineSemaphore::getDispatchableHandle() const { return (uint64)semaphore; }

void VulkanTimelineSemaphore::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanTimelineSemaphore::reinitResources()
{
    release();
    BaseType::reinitResources();
    if (!GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get())
    {
        LOG_WARN("VulkanTimelineSemaphore", "Cannot use timeline semaphore as feature is not supported");
        semaphore = nullptr;
        return;
    }
    fatalAssertf(ownerDevice && vulkanDevice, "Required devices cannot be null");
    VkSemaphore nextSemaphore;

    CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
    CREATE_TYPED_SEMAPHORE_INFO(typedSemaphoreCreateInfo);
    semaphoreCreateInfo.pNext = &typedSemaphoreCreateInfo;

    if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore) == VK_SUCCESS)
    {
        semaphore = nextSemaphore;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        LOG_ERROR("VulkanSemaphore", "Reinit failed to create new semaphore");
    }
}

void VulkanTimelineSemaphore::release()
{
    fatalAssertf(ownerDevice && vulkanDevice, "Required devices cannot be null");
    if (semaphore)
    {
        vulkanDevice->vkDestroySemaphore(ownerDevice, semaphore, nullptr);
        semaphore = nullptr;
    }
    BaseType::release();
}

//////////////////////////////////////////////////////////////////////////
// VulkanFence
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanFence, VK_OBJECT_TYPE_FENCE)

VulkanFence::VulkanFence(const VulkanDevice *deviceInstance, bool bIsSignaled)
    : BaseType()
    , vulkanDevice(deviceInstance)
    , ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance))
    , bCreateSignaled(bIsSignaled)
    , fence(nullptr)
{}

void VulkanFence::waitForSignal() const
{
    VkResult result = vulkanDevice->vkWaitForFences(ownerDevice, 1, &fence, VK_TRUE, GlobalRenderVariables::MAX_SYNC_RES_WAIT_TIME.get());

    if (result == VK_TIMEOUT)
    {
        LOG_WARN("VulkanFence", "waiting for fence timedout");
    }
}

bool VulkanFence::isSignaled() const { return vulkanDevice->vkGetFenceStatus(ownerDevice, fence) == VK_SUCCESS; }

void VulkanFence::resetSignal() { vulkanDevice->vkResetFences(ownerDevice, 1, &fence); }

void VulkanFence::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanFence::reinitResources()
{
    release();
    BaseType::reinitResources();
    fatalAssertf(ownerDevice && vulkanDevice, "Required devices cannot be null");
    VkFence nextFence;

    CREATE_FENCE_INFO(fenceCreateInfo);
    fenceCreateInfo.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    if (vulkanDevice->vkCreateFence(ownerDevice, &fenceCreateInfo, nullptr, &nextFence) == VK_SUCCESS)
    {
        fence = nextFence;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        LOG_ERROR("VulkanFence", "Failed recreating fence");
    }
}

void VulkanFence::release()
{
    if (fence && fence != VK_NULL_HANDLE)
    {
        vulkanDevice->vkDestroyFence(ownerDevice, fence, nullptr);
        fence = nullptr;
    }
    BaseType::release();
}

String VulkanFence::getObjectName() const { return getResourceName(); }

uint64 VulkanFence::getDispatchableHandle() const { return (uint64)fence; }

//////////////////////////////////////////////////////////////////////////
// VulkanEvent
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanEvent, VK_OBJECT_TYPE_EVENT)

bool VulkanEvent::isSignaled() const
{
    debugAssertf(!bDeviceOnly, "Trying to get state of device only event!");
    if (!bDeviceOnly && vulkanEvent != VK_NULL_HANDLE)
    {
        return vulkanDevice->vkGetEventStatus(ownerDevice, vulkanEvent) == VkResult::VK_EVENT_SET;
    }
    return false;
}

void VulkanEvent::resetSignal()
{
    if (vulkanEvent != VK_NULL_HANDLE)
    {
        vulkanDevice->vkResetEvent(ownerDevice, vulkanEvent);
    }
}

void VulkanEvent::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanEvent::reinitResources()
{
    release();
    BaseType::reinitResources();

    fatalAssertf(ownerDevice && vulkanDevice, "Required devices cannot be null");
    VkEvent newEvent;

    CREATE_EVENT_INFO(eventCreateInfo);
    eventCreateInfo.flags = bDeviceOnly ? VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR : 0;
    if (vulkanDevice->vkCreateEvent(ownerDevice, &eventCreateInfo, nullptr, &newEvent) == VK_SUCCESS)
    {
        vulkanEvent = newEvent;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        LOG_ERROR("VulkanFence", "Failed recreating event {}", getResourceName());
    }
}

void VulkanEvent::release()
{
    if (vulkanEvent && vulkanEvent != VK_NULL_HANDLE)
    {
        vulkanDevice->vkDestroyEvent(ownerDevice, vulkanEvent, nullptr);
        vulkanEvent = nullptr;
    }
    BaseType::release();
}

String VulkanEvent::getObjectName() const { return getResourceName(); }

uint64 VulkanEvent::getDispatchableHandle() const { return (uint64)vulkanEvent; }
