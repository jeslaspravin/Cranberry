/*!
 * \file VulkanSyncResource.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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

void VulkanSemaphore::waitForSignal() const
{
    LOG_WARN("VulkanSemaphore", "%s() : Cannot wait on binary semaphores from host", __func__);
}

bool VulkanSemaphore::isSignaled() const
{
    LOG_WARN("VulkanSemaphore", "%s() : Cannot check state on binary semaphores from host", __func__);
    return false;
}

void VulkanSemaphore::resetSignal()
{
    LOG_WARN("VulkanSemaphore", "%s() : Cannot reset state on binary semaphores from host", __func__);
}

void VulkanSemaphore::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanSemaphore::reinitResources()
{
    release();
    BaseType::reinitResources();
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
    VkSemaphore nextSemaphore;

    CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
    if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore)
        == VK_SUCCESS)
    {
        semaphore = nextSemaphore;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        LOG_ERROR("VulkanSemaphore", "%s() : Reinit failed to create new semaphore", __func__);
    }
}

void VulkanSemaphore::release()
{
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
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
        vulkanDevice->TIMELINE_SEMAPHORE_TYPE(vkWaitSemaphores)(
            ownerDevice, &waitInfo, GlobalRenderVariables::MAX_SYNC_RES_WAIT_TIME.get());
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

        if (vulkanDevice->TIMELINE_SEMAPHORE_TYPE(vkSignalSemaphore)(ownerDevice, &signalInfo)
            != VK_SUCCESS)
        {
            LOG_ERROR("VulkanTimelineSemaphore", "%s() : Signaling to value %d failed", __func__, value);
        }
    }
}

uint64 VulkanTimelineSemaphore::currentValue() const
{
    uint64 counter = 0;
    if (GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get())
    {
        vulkanDevice->TIMELINE_SEMAPHORE_TYPE(vkGetSemaphoreCounterValue)(
            ownerDevice, semaphore, &counter);
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
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
    VkSemaphore nextSemaphore;

    CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
    CREATE_TYPED_SEMAPHORE_INFO(typedSemaphoreCreateInfo);
    semaphoreCreateInfo.pNext = &typedSemaphoreCreateInfo;

    if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore)
        == VK_SUCCESS)
    {
        semaphore = nextSemaphore;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        LOG_ERROR("VulkanSemaphore", "%s() : Reinit failed to create new semaphore", __func__);
    }
}

void VulkanTimelineSemaphore::release()
{
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
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
    VkResult result = vulkanDevice->vkWaitForFences(
        ownerDevice, 1, &fence, VK_TRUE, GlobalRenderVariables::MAX_SYNC_RES_WAIT_TIME.get());

    if (result == VK_TIMEOUT)
    {
        LOG_WARN("VulkanFence", "%s() : waiting for fence timedout", __func__);
    }
}

bool VulkanFence::isSignaled() const
{
    return vulkanDevice->vkGetFenceStatus(ownerDevice, fence) == VK_SUCCESS;
}

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
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
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
        LOG_ERROR("VulkanFence", "%s() : Failed recreating fence", __func__);
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
