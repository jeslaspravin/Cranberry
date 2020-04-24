#include "VulkanSyncResource.h"
#include "../VulkanDevice.h"
#include "../../../Core/Logger/Logger.h"
#include "../../VulkanGraphicsHelper.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"

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
    BaseType::init();
    reinitResources();
}

void VulkanSemaphore::reinitResources()
{
    release();
    BaseType::reinitResources();
    fatalAssert(ownerDevice && vulkanDevice,"Required devices cannot be null");
    VkSemaphore nextSemaphore;

    CREATE_SEMAPHORE_INFO(semaphoreCreateInfo);
    if (vulkanDevice->vkCreateSemaphore(ownerDevice, &semaphoreCreateInfo, nullptr, &nextSemaphore) == VK_SUCCESS)
    {
        semaphore = nextSemaphore;
        vulkanDevice->debugGraphics()->markObject(this);
    }
    else
    {
        Logger::error("VulkanSemaphore", "%s() : Reinit failed to create new semaphore", __func__);
    }
}

void VulkanSemaphore::release()
{
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
    if (semaphore)
    {
        vulkanDevice->vkDestroySemaphore(ownerDevice, semaphore, nullptr);
    }
    BaseType::release();
}

String VulkanSemaphore::getObjectName() const
{
    return getResourceName();
}

void VulkanSemaphore::setObjectName(const String& name)
{
    setResourceName(name);
}

uint64 VulkanSemaphore::getDispatchableHandle() const
{
    return (uint64)semaphore;
}

//////////////////////////////////////////////////////////////////////////
// VulkanTimelineSemaphore 

DEFINE_VK_GRAPHICS_RESOURCE(VulkanTimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE)

// TODO(Jeslas)(API Update) : Change and remove this macro once driver providers update to Vulkan 1.2
#if 0 
#define TIMIELINE_SEMAPHORE_FUNCTIONS(VDevice,FunctionName) VDevice->FunctionName
#else
#define TIMIELINE_SEMAPHORE_FUNCTIONS(VDevice,FunctionName) VDevice->FunctionName##KHR
#endif                      


VulkanTimelineSemaphore::VulkanTimelineSemaphore(const VulkanDevice* deviceInstance)
    :BaseType(), ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance)), vulkanDevice(deviceInstance)
{}

void VulkanTimelineSemaphore::waitForSignal(uint64 value) const
{
    if (!isSignaled(value) && GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get())
    {
        SEMAPHORE_WAIT_INFO(waitInfo);
        waitInfo.pSemaphores = &semaphore;
        waitInfo.semaphoreCount = 1;
        waitInfo.pValues = &value;
        TIMIELINE_SEMAPHORE_FUNCTIONS(vulkanDevice,vkWaitSemaphores)(ownerDevice, &waitInfo, GlobalRenderVariables::MAX_SYNC_RES_WAIT_TIME.get());
    }
}

bool VulkanTimelineSemaphore::isSignaled(uint64 value) const
{
    return value >= currentValue();
}

void VulkanTimelineSemaphore::resetSignal(uint64 value)
{
    uint64 currentVal = currentValue();
    if (GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get() && value > currentVal
        && (value - currentVal) < GlobalRenderVariables::MAX_TIMELINE_OFFSET.get())
    {
        SEMAPHORE_SIGNAL_INFO(signalInfo);
        signalInfo.semaphore = semaphore;
        signalInfo.value = value;

        if (TIMIELINE_SEMAPHORE_FUNCTIONS(vulkanDevice,vkSignalSemaphore)(ownerDevice, &signalInfo) != VK_SUCCESS)
        {
            Logger::error("VulkanTimelineSemaphore", "%s() : Signaling to value %d failed", __func__, value);
        }
    }
}

uint64 VulkanTimelineSemaphore::currentValue() const
{
    uint64 counter = 0;
    if(GlobalRenderVariables::ENABLED_TIMELINE_SEMAPHORE.get())
    {
        TIMIELINE_SEMAPHORE_FUNCTIONS(vulkanDevice,vkGetSemaphoreCounterValue)(ownerDevice, semaphore, &counter);
    }
    return counter;
}

String VulkanTimelineSemaphore::getObjectName() const
{
    return getResourceName();
}

void VulkanTimelineSemaphore::setObjectName(const String& name)
{
    setResourceName(name);
}

uint64 VulkanTimelineSemaphore::getDispatchableHandle() const
{
    return (uint64)semaphore;
}

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
        Logger::warn("VulkanTimelineSemaphore", "Cannot use timeline semaphore as feature is not supported");
        semaphore = nullptr;
        return;
    }
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
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
        Logger::error("VulkanSemaphore", "%s() : Reinit failed to create new semaphore", __func__);
    }
}

void VulkanTimelineSemaphore::release()
{
    fatalAssert(ownerDevice && vulkanDevice, "Required devices cannot be null");
    if (semaphore)
    {
        vulkanDevice->vkDestroySemaphore(ownerDevice, semaphore, nullptr);
    }
    BaseType::release();
}

#undef TIMIELINE_SEMAPHORE_FUNCTIONS

//////////////////////////////////////////////////////////////////////////
// VulkanFence

DEFINE_VK_GRAPHICS_RESOURCE(VulkanFence, VK_OBJECT_TYPE_FENCE)

VulkanFence::VulkanFence(const VulkanDevice* deviceInstance, bool bIsSignaled)
    :BaseType()
    , vulkanDevice(deviceInstance)
    , ownerDevice(VulkanGraphicsHelper::getDevice(deviceInstance))
    , bCreateSignaled(bIsSignaled)
{}

void VulkanFence::waitForSignal() const
{
    VkResult result = vulkanDevice->vkWaitForFences(ownerDevice, 1, &fence, VK_TRUE, GlobalRenderVariables::MAX_SYNC_RES_WAIT_TIME.get());

    if (result == VK_TIMEOUT)
    {
        Logger::warn("VulkanFence", "%s() : waiting for fence timedout", __func__);
    }
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
        Logger::error("VulkanFence", "%s() : Failed recreating fence", __func__);
    }
}

void VulkanFence::release()
{
    if (fence && fence != VK_NULL_HANDLE)
    {
        waitForSignal();
        vulkanDevice->vkDestroyFence(ownerDevice, fence, nullptr);
        fence = nullptr;
    }
    BaseType::release();
}

String VulkanFence::getObjectName() const
{
    return getResourceName();
}

void VulkanFence::setObjectName(const String& name)
{
    setResourceName(name);
}

uint64 VulkanFence::getDispatchableHandle() const
{
    return (uint64)fence;
}
