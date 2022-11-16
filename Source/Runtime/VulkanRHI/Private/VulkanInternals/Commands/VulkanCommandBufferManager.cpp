/*!
 * \file VulkanCommandBufferManager.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <variant>
#include <unordered_set>

#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Types/Templates/TemplateTypes.h"
#include "VulkanInternals/Commands/VulkanCommandBufferManager.h"
#include "VulkanInternals/Resources/VulkanMemoryResources.h"
#include "VulkanInternals/Resources/VulkanQueueResource.h"
#include "VulkanInternals/Resources/VulkanSyncResource.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanRHIModule.h"

template <typename QueueResType, EQueuePriority::Enum Priority>
struct GetQueueOfPriority
{
    VkQueue operator()(QueueResType *queueRes) { return queueRes->getQueueOfPriority<Priority>(); }
};

template <typename QueueResType>
using GetQueueOfPriorityLow = GetQueueOfPriority<QueueResType, EQueuePriority::Low>;
template <typename QueueResType>
using GetQueueOfPriorityMedium = GetQueueOfPriority<QueueResType, EQueuePriority::Medium>;
template <typename QueueResType>
using GetQueueOfPriorityHigh = GetQueueOfPriority<QueueResType, EQueuePriority::High>;
template <typename QueueResType>
using GetQueueOfPrioritySuperHigh = GetQueueOfPriority<QueueResType, EQueuePriority::SuperHigh>;

template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction> *getQueue(const VulkanDevice *device);

//////////////////////////////////////////////////////////////////////////
////  VulkanCommandBuffer
//////////////////////////////////////////////////////////////////////////

class VulkanCommandBuffer final
    : public GraphicsResource
    , public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCommandBuffer, , GraphicsResource, );

private:
    String bufferName;

public:
    VkCommandBuffer cmdBuffer;
    bool bIsResetable = false;
    bool bIsTempBuffer = false;
    EQueueFunction fromQueue;
    EQueueFunction usage;

    /* GraphicsResource overrides */
    String getResourceName() const override;
    void setResourceName(const String &name) override;
    /* IVulkanResources overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    /* Override ends */
};

#if EXPERIMENTAL

VkCommandBuffer VulkanGraphicsHelper::getRawCmdBuffer(class IGraphicsInstance * /*graphicsInstance*/, const GraphicsResource *cmdBuffer)
{
    if (cmdBuffer->getType()->isChildOf<VulkanCommandBuffer>())
    {
        return static_cast<const VulkanCommandBuffer *>(cmdBuffer)->cmdBuffer;
    }
    return nullptr;
}

#endif

DEFINE_VK_GRAPHICS_RESOURCE(VulkanCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER)

String VulkanCommandBuffer::getObjectName() const { return getResourceName(); }

String VulkanCommandBuffer::getResourceName() const { return bufferName; }

void VulkanCommandBuffer::setResourceName(const String &name) { bufferName = name; }

uint64 VulkanCommandBuffer::getDispatchableHandle() const { return uint64(cmdBuffer); }

//////////////////////////////////////////////////////////////////////////
////  VulkanCommandPool
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanCommandPool, VK_OBJECT_TYPE_COMMAND_POOL)

void VulkanCommandPool::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanCommandPool::reinitResources()
{
    if (cmdPoolInfo.queueResource == nullptr)
    {
        LOG_ERROR("VulkanCommandPool", "Command pool information is invalid");
        return;
    }
    release();
    BaseType::reinitResources();

    CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
    commandPoolCreateInfo.queueFamilyIndex = cmdPoolInfo.vulkanQueueIndex;

    commandPoolCreateInfo.flags = 0;
    if (cmdPoolInfo.vDevice->vkCreateCommandPool(cmdPoolInfo.logicalDevice, &commandPoolCreateInfo, nullptr, &oneTimeRecordPool) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanCommandPool", "Failed creating one time record command buffer pool");
        oneTimeRecordPool = nullptr;
    }
    else
    {
        cmdPoolInfo.vDevice->debugGraphics()->markObject(
            (uint64)oneTimeRecordPool, getResourceName()
                                           + TCHAR("_OneTimeRecordPool"), getObjectType()
                                       );
    }

    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if (cmdPoolInfo.vDevice->vkCreateCommandPool(cmdPoolInfo.logicalDevice, &commandPoolCreateInfo, nullptr, &tempCommandsPool) != VK_SUCCESS)
    {
        LOG_ERROR("VulkanCommandPool", "Failed creating temporary one time use command buffer pool");
        tempCommandsPool = nullptr;
    }
    else
    {
        cmdPoolInfo.vDevice->debugGraphics()->markObject((uint64)tempCommandsPool, getResourceName() + TCHAR("_TempCmdsPool"), getObjectType());
    }

    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (cmdPoolInfo.vDevice->vkCreateCommandPool(cmdPoolInfo.logicalDevice, &commandPoolCreateInfo, nullptr, &rerecordableCommandPool)
        != VK_SUCCESS)
    {
        LOG_ERROR("VulkanCommandPool", "Failed creating rerecordable command buffer pool");
        rerecordableCommandPool = nullptr;
    }
    else
    {
        cmdPoolInfo.vDevice->debugGraphics()->markObject(
            (uint64)rerecordableCommandPool, getResourceName()
                                                 + TCHAR("_RerecordableCmdPool"), getObjectType()
                                             );
    }
}

void VulkanCommandPool::release()
{
    if (oneTimeRecordPool)
    {
        cmdPoolInfo.vDevice->vkResetCommandPool(
            cmdPoolInfo.logicalDevice, oneTimeRecordPool, VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
        );
        cmdPoolInfo.vDevice->vkDestroyCommandPool(cmdPoolInfo.logicalDevice, oneTimeRecordPool, nullptr);
        oneTimeRecordPool = nullptr;
    }
    if (rerecordableCommandPool)
    {
        cmdPoolInfo.vDevice->vkResetCommandPool(
            cmdPoolInfo.logicalDevice, rerecordableCommandPool, VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
        );
        cmdPoolInfo.vDevice->vkDestroyCommandPool(cmdPoolInfo.logicalDevice, rerecordableCommandPool, nullptr);
        rerecordableCommandPool = nullptr;
    }
    if (tempCommandsPool)
    {
        cmdPoolInfo.vDevice->vkResetCommandPool(
            cmdPoolInfo.logicalDevice, tempCommandsPool, VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
        );
        cmdPoolInfo.vDevice->vkDestroyCommandPool(cmdPoolInfo.logicalDevice, tempCommandsPool, nullptr);
        tempCommandsPool = nullptr;
    }

    BaseType::release();
}

String VulkanCommandPool::getResourceName() const { return poolName; }

void VulkanCommandPool::setResourceName(const String &name) { poolName = name; }

String VulkanCommandPool::getObjectName() const { return getResourceName(); }

VkCommandPool VulkanCommandPool::getCommandPool(class VulkanCommandBuffer const *cmdBuffer) const
{
    VkCommandPool vCmdPool = nullptr;

    if (cmdBuffer->bIsResetable)
    {
        vCmdPool = rerecordableCommandPool;
    }
    else if (cmdBuffer->bIsTempBuffer)
    {
        vCmdPool = tempCommandsPool;
    }
    else
    {
        vCmdPool = oneTimeRecordPool;
    }
    return vCmdPool;
}

//////////////////////////////////////////////////////////////////////////
//// VulkanCmdBufferManager
//////////////////////////////////////////////////////////////////////////

VulkanCmdBufferManager::VulkanCmdBufferManager(class VulkanDevice *vulkanDevice)
    : vDevice(vulkanDevice)
{
    createPools();
}

VulkanCmdBufferManager::~VulkanCmdBufferManager()
{
    for (const std::pair<const String, VulkanCmdBufferState> &cmdBuffer : commandBuffers)
    {
        if (cmdBuffer.second.cmdSyncInfoIdx != -1)
        {
            LOG_WARN(
                "VulkanCmdBufferManager", "Command buffer %s is not finished, trying to finish it",
                cmdBuffer.second.cmdBuffer->getResourceName().getChar()
            );
            cmdFinished(cmdBuffer.second.cmdBuffer->getResourceName(), nullptr);
        }
        cmdBuffer.second.cmdBuffer->release();
        delete cmdBuffer.second.cmdBuffer;
    }
    for (std::pair<const EQueueFunction, VulkanCommandPool> &poolPair : pools)
    {
        poolPair.second.release();
    }
    pools.clear();
}

const GraphicsResource *VulkanCmdBufferManager::beginTempCmdBuffer(const String &cmdName, EQueueFunction usingQueue)
{
    VulkanCommandPool &cmdPool = getPool(usingQueue);

    CMD_BUFFER_ALLOC_INFO(cmdBuffAllocInfo);
    cmdBuffAllocInfo.commandPool = cmdPool.tempCommandsPool;
    cmdBuffAllocInfo.commandBufferCount = 1;

    auto *cmdBuffer = new VulkanCommandBuffer();
    cmdBuffer->setResourceName(cmdName);
    cmdBuffer->bIsTempBuffer = true;
    cmdBuffer->fromQueue = cmdPool.cmdPoolInfo.queueType;
    cmdBuffer->usage = usingQueue;

    fatalAssertf(
        vDevice->vkAllocateCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), &cmdBuffAllocInfo, &cmdBuffer->cmdBuffer) == VK_SUCCESS,
        "Allocating temporary command buffer failed"
    );
    cmdBuffer->init();
    vDevice->debugGraphics()->markObject(cmdBuffer);

    CMD_BUFFER_BEGIN_INFO(cmdBuffBeginInfo);
    cmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(cmdBuffer->cmdBuffer, &cmdBuffBeginInfo);
    vDevice->debugGraphics()->beginCmdBufferMarker(cmdBuffer->cmdBuffer, cmdName);

    return cmdBuffer;
}

const GraphicsResource *VulkanCmdBufferManager::beginRecordOnceCmdBuffer(const String &cmdName, EQueueFunction usingQueue)
{
    VulkanCommandBuffer *cmdBuffer = nullptr;

    auto cmdBufferItr = commandBuffers.find(cmdName);
    if (cmdBufferItr == commandBuffers.end())
    {
        VulkanCommandPool &cmdPool = getPool(usingQueue);

        CMD_BUFFER_ALLOC_INFO(cmdBuffAllocInfo);
        cmdBuffAllocInfo.commandPool = cmdPool.oneTimeRecordPool;
        cmdBuffAllocInfo.commandBufferCount = 1;

        cmdBuffer = new VulkanCommandBuffer();
        cmdBuffer->setResourceName(cmdName);
        cmdBuffer->fromQueue = cmdPool.cmdPoolInfo.queueType;
        cmdBuffer->usage = usingQueue;

        fatalAssertf(
            vDevice->vkAllocateCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), &cmdBuffAllocInfo, &cmdBuffer->cmdBuffer) == VK_SUCCESS,
            "Allocating record once command buffer failed"
        );
        cmdBuffer->init();
        vDevice->debugGraphics()->markObject(cmdBuffer);

        commandBuffers[cmdName] = { cmdBuffer, ECmdState::Recording };
    }
    else
    {
        switch (cmdBufferItr->second.cmdState)
        {
        case ECmdState::Recorded:
        case ECmdState::Submitted:
            LOG_ERROR(
                "VulkanCommandBufferManager",
                "Trying to record a prerecorded command again is restricted Command = "
                "[%s]",
                cmdName.getChar()
            );
            fatalAssertf(false, "Cannot record prerecorded command again");
            return cmdBufferItr->second.cmdBuffer;
        case ECmdState::Recording:
            LOG_WARN("VulkanCommandBufferManager", "Command %s is already being recorded", cmdName.getChar());
            return cmdBufferItr->second.cmdBuffer;
        case ECmdState::Idle:
        default:
            cmdBuffer = cmdBufferItr->second.cmdBuffer;
        }
        debugAssert(!cmdBuffer->bIsResetable);
    }

    CMD_BUFFER_BEGIN_INFO(cmdBuffBeginInfo);
    cmdBuffBeginInfo.flags = 0;

    vDevice->vkBeginCommandBuffer(cmdBuffer->cmdBuffer, &cmdBuffBeginInfo);
    return cmdBuffer;
}

const GraphicsResource *VulkanCmdBufferManager::beginReuseCmdBuffer(const String &cmdName, EQueueFunction usingQueue)
{
    VulkanCommandBuffer *cmdBuffer = nullptr;

    auto cmdBufferItr = commandBuffers.find(cmdName);
    if (cmdBufferItr == commandBuffers.end())
    {
        VulkanCommandPool &cmdPool = getPool(usingQueue);

        CMD_BUFFER_ALLOC_INFO(cmdBuffAllocInfo);
        cmdBuffAllocInfo.commandPool = cmdPool.rerecordableCommandPool;
        cmdBuffAllocInfo.commandBufferCount = 1;

        cmdBuffer = new VulkanCommandBuffer();
        cmdBuffer->setResourceName(cmdName);
        cmdBuffer->bIsResetable = true;
        cmdBuffer->fromQueue = cmdPool.cmdPoolInfo.queueType;
        cmdBuffer->usage = usingQueue;

        fatalAssertf(
            vDevice->vkAllocateCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), &cmdBuffAllocInfo, &cmdBuffer->cmdBuffer) == VK_SUCCESS,
            "Allocating reusable command buffer failed"
        );
        cmdBuffer->init();
        vDevice->debugGraphics()->markObject(cmdBuffer);

        commandBuffers[cmdName] = { cmdBuffer, ECmdState::Recording };
    }
    else
    {
        switch (cmdBufferItr->second.cmdState)
        {
        case ECmdState::Submitted:
            LOG_ERROR(
                "VulkanCommandBufferManager",
                "Trying to record a submitted command [%s] is restricted before it is "
                "finished",
                cmdName.getChar()
            );
            fatalAssertf(false, "Cannot record command while it is still executing");
            return cmdBufferItr->second.cmdBuffer;
        case ECmdState::Recording:
            LOG_WARN("VulkanCommandBufferManager", "Command [%s] is already being recorded", cmdName.getChar());
            return cmdBufferItr->second.cmdBuffer;
        case ECmdState::Recorded:
        case ECmdState::Idle:
        default:
            cmdBuffer = cmdBufferItr->second.cmdBuffer;
        }

        debugAssert(cmdBuffer->bIsResetable);
        cmdBufferItr->second.cmdState = ECmdState::Recording;
    }

    CMD_BUFFER_BEGIN_INFO(cmdBuffBeginInfo);
    cmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(cmdBuffer->cmdBuffer, &cmdBuffBeginInfo);
    return cmdBuffer;
}

void VulkanCmdBufferManager::startRenderPass(const GraphicsResource *cmdBuffer)
{
    const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
        if (cmdBufferItr != commandBuffers.end())
        {
            fatalAssertf(
                cmdBufferItr->second.cmdState == ECmdState::Recording, "%s cmd buffer is not recording to start render pass",
                cmdBufferItr->first.getChar()
            );
            cmdBufferItr->second.cmdState = ECmdState::RenderPass;
        }
    }
}

bool VulkanCmdBufferManager::isInRenderPass(const GraphicsResource *cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    if (cmdBufferItr != commandBuffers.end())
    {
        return cmdBufferItr->second.cmdState == ECmdState::RenderPass;
    }

    return false;
}

void VulkanCmdBufferManager::endRenderPass(const GraphicsResource *cmdBuffer)
{
    const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
        if (cmdBufferItr != commandBuffers.end() && cmdBufferItr->second.cmdState == ECmdState::RenderPass)
        {
            cmdBufferItr->second.cmdState = ECmdState::Recording;
        }
    }
}

void VulkanCmdBufferManager::endCmdBuffer(const GraphicsResource *cmdBuffer)
{
    const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Recorded;
    }
    else
    {
        vDevice->debugGraphics()->endCmdBufferMarker(vCmdBuffer->cmdBuffer);
    }
    vDevice->vkEndCommandBuffer(vCmdBuffer->cmdBuffer);
}

void VulkanCmdBufferManager::cmdFinished(const GraphicsResource *cmdBuffer, VulkanResourcesTracker *resourceTracker)
{
    cmdFinished(cmdBuffer->getResourceName(), resourceTracker);
}

void VulkanCmdBufferManager::cmdFinished(const String &cmdName, VulkanResourcesTracker *resourceTracker)
{
    auto cmdBufferItr = commandBuffers.find(cmdName);
    // If submitted then only it can be finished in queue
    if (cmdBufferItr != commandBuffers.end() && cmdBufferItr->second.cmdState == ECmdState::Submitted)
    {
        VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
        syncInfo.refCount--;

        fatalAssertf(syncInfo.completeFence.isValid(), "Complete fence cannot be null!");
        if (!syncInfo.completeFence->isSignaled())
        {
            syncInfo.completeFence->waitForSignal();
        }

        // wait until other cmd buffers waiting on this is complete before cleaning resources
        if (resourceTracker)
        {
            for (const GraphicsResource *cmdBuf : resourceTracker->getDependingCmdBuffers(cmdBufferItr->second.cmdBuffer))
            {
                cmdFinished(cmdBuf, resourceTracker);
            }
            resourceTracker->clearFinishedCmd(cmdBufferItr->second.cmdBuffer);
        }

        // Reset resources
        if (syncInfo.refCount == 0)
        {
            syncInfo.completeFence->resetSignal();
            syncInfo.completeFence.reset();
            syncInfo.signalingSemaphore.reset();

            cmdsSyncInfo.reset(cmdBufferItr->second.cmdSyncInfoIdx);
        }
        cmdBufferItr->second.cmdSyncInfoIdx = -1;
        cmdBufferItr->second.cmdState = ECmdState::Recorded;
    }
}

void VulkanCmdBufferManager::finishAllSubmited(VulkanResourcesTracker *resourceTracker)
{
    for (const std::pair<const String, VulkanCmdBufferState> &cmd : commandBuffers)
    {
        if (cmd.second.cmdState == ECmdState::Submitted)
        {
            VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmd.second.cmdSyncInfoIdx];
            if (!syncInfo.completeFence->isSignaled())
            {
                syncInfo.completeFence->waitForSignal();
            }
            cmdFinished(cmd.second.cmdBuffer->getResourceName(), resourceTracker);
        }
    }
}

void VulkanCmdBufferManager::freeCmdBuffer(const GraphicsResource *cmdBuffer)
{
    const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(cmdBuffer);
    VulkanCommandPool &cmdPool = getPool(vCmdBuffer->fromQueue);

    vDevice->vkFreeCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), cmdPool.getCommandPool(vCmdBuffer), 1, &vCmdBuffer->cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        commandBuffers.erase(cmdBuffer->getResourceName());
    }

    const_cast<GraphicsResource *>(cmdBuffer)->release();
    delete cmdBuffer;
}

VkCommandBuffer VulkanCmdBufferManager::getRawBuffer(const GraphicsResource *cmdBuffer) const
{
    return cmdBuffer->getType()->isChildOf<VulkanCommandBuffer>() ? static_cast<const VulkanCommandBuffer *>(cmdBuffer)->cmdBuffer : nullptr;
}

const GraphicsResource *VulkanCmdBufferManager::getCmdBuffer(const String &cmdName) const
{
    auto cmdBufferItr = commandBuffers.find(cmdName);
    if (cmdBufferItr != commandBuffers.end())
    {
        return cmdBufferItr->second.cmdBuffer;
    }
    return nullptr;
}

uint32 VulkanCmdBufferManager::getQueueFamilyIdx(EQueueFunction queue) const { return pools.find(queue)->second.cmdPoolInfo.vulkanQueueIndex; }

uint32 VulkanCmdBufferManager::getQueueFamilyIdx(const GraphicsResource *cmdBuffer) const
{
    return getQueueFamilyIdx(static_cast<const VulkanCommandBuffer *>(cmdBuffer)->fromQueue);
}

EQueueFunction VulkanCmdBufferManager::getQueueFamily(uint32 familyIdx) const
{
    for (const std::pair<const EQueueFunction, VulkanCommandPool> &poolPair : pools)
    {
        if (poolPair.second.cmdPoolInfo.vulkanQueueIndex == familyIdx)
        {
            return poolPair.first;
        }
    }
    debugAssertf(false, "Invalide queue family index %u", familyIdx);
    return EQueueFunction::Generic;
}

ECmdState VulkanCmdBufferManager::getState(const GraphicsResource *cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    if (cmdBufferItr != commandBuffers.cend())
    {
        return cmdBufferItr->second.cmdState;
    }
    LOG_DEBUG("VulkanCmdBufferManager", "Not available command buffer[%s] queried for state", cmdBuffer->getResourceName().getChar());
    return ECmdState::Idle;
}

TimelineSemaphoreRef VulkanCmdBufferManager::cmdSignalSemaphore(const GraphicsResource *cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    if (cmdBufferItr != commandBuffers.cend() && cmdBufferItr->second.cmdSyncInfoIdx >= 0)
    {
        return cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx].signalingSemaphore;
    }
    return nullptr;
}

bool VulkanCmdBufferManager::isComputeCmdBuffer(const GraphicsResource *cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer *>(cmdBuffer)->usage == EQueueFunction::Compute;
}

bool VulkanCmdBufferManager::isGraphicsCmdBuffer(const GraphicsResource *cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer *>(cmdBuffer)->usage == EQueueFunction::Graphics;
}

bool VulkanCmdBufferManager::isTransferCmdBuffer(const GraphicsResource *cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer *>(cmdBuffer)->usage == EQueueFunction::Transfer;
}

EQueueFunction VulkanCmdBufferManager::getCmdBufferQueue(const GraphicsResource *cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer *>(cmdBuffer)->usage;
}

bool VulkanCmdBufferManager::isCmdFinished(const GraphicsResource *cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    // If submitted then only it can be finished in queue
    if (cmdBufferItr != commandBuffers.end() && cmdBufferItr->second.cmdState == ECmdState::Submitted)
    {
        const VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];

        fatalAssertf(syncInfo.completeFence.isValid(), "Complete fence cannot be null!");
        return syncInfo.completeFence->isSignaled();
    }
    return true;
}

void VulkanCmdBufferManager::submitCmds(EQueuePriority::Enum priority, ArrayView<const CommandSubmitInfo> commands, FenceRef cmdsCompleteFence)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();
    QueueResourceBase *queueRes = nullptr;

    std::vector<TimelineSemaphoreRef> managerTSemaphores(commands.size());
    std::vector<std::vector<VkCommandBufferSubmitInfo>> allCmdBuffers(commands.size());
    std::vector<std::vector<VkSemaphoreSubmitInfo>> allWaitOnSemaphores(commands.size());
    std::vector<std::vector<VkSemaphoreSubmitInfo>> allSignallingSemaphores(commands.size());
    std::vector<VkSubmitInfo2> allSubmitInfo;
    allSubmitInfo.reserve(commands.size());

    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        std::vector<VkCommandBufferSubmitInfo> &cmdBuffers = allCmdBuffers[cmdSubmitIdx];
        cmdBuffers.reserve(commands[cmdSubmitIdx].cmdBuffers.size());
        std::vector<VkSemaphoreSubmitInfo> &waitOnSemaphores = allWaitOnSemaphores[cmdSubmitIdx];
        waitOnSemaphores.reserve(commands[cmdSubmitIdx].waitOn.size() + commands[cmdSubmitIdx].waitOnTimelines.size());
        std::vector<VkSemaphoreSubmitInfo> &signalingSemaphores = allSignallingSemaphores[cmdSubmitIdx];
        signalingSemaphores.reserve(commands[cmdSubmitIdx].signalSemaphores.size() + commands[cmdSubmitIdx].signalTimelines.size() + 1);

        bool bHasNonTemp = false;
        for (int32 i = 0; i < commands[cmdSubmitIdx].cmdBuffers.size(); ++i)
        {
            const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(commands[cmdSubmitIdx].cmdBuffers[i]);
            bHasNonTemp = bHasNonTemp || !vCmdBuffer->bIsTempBuffer;

            CMDBUFFER_SUBMIT_INFO(cmdBufferSubmitInfo);
            cmdBufferSubmitInfo.commandBuffer = vCmdBuffer->cmdBuffer;
            cmdBuffers.emplace_back(std::move(cmdBufferSubmitInfo));

            VulkanCommandPool &cmdPool = getPool(vCmdBuffer->fromQueue);
            if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
            {
                LOG_ERROR("VulkanCommandBufferManager", "Buffers from different queues cannot be submitted together");
                return;
            }
            queueRes = cmdPool.cmdPoolInfo.queueResource;
        }
        if (queueRes == nullptr)
        {
            LOG_ERROR("VulkanCommandBufferManager", "Cannot submit as there is no queue found for command buffers");
            return;
        }

        for (int32 i = 0; i < commands[cmdSubmitIdx].waitOn.size(); ++i)
        {
            SEMAPHORE_SUBMIT_INFO(waitOnSema);
            waitOnSema.semaphore = commands[cmdSubmitIdx].waitOn[i].semaphore.reference<VulkanSemaphore>()->semaphore;
            waitOnSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(commands[cmdSubmitIdx].waitOn[i].stages);
            waitOnSemaphores.emplace_back(std::move(waitOnSema));
        }
        for (int32 i = 0; i < commands[cmdSubmitIdx].waitOnTimelines.size(); ++i)
        {
            SEMAPHORE_SUBMIT_INFO(waitOnSema);
            waitOnSema.semaphore = commands[cmdSubmitIdx].waitOnTimelines[i].semaphore.reference<VulkanTimelineSemaphore>()->semaphore;
            waitOnSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(commands[cmdSubmitIdx].waitOnTimelines[i].stages);
            waitOnSema.value = commands[cmdSubmitIdx].waitOnTimelines[i].value;
            waitOnSemaphores.emplace_back(std::move(waitOnSema));
        }
        for (int32 i = 0; i < commands[cmdSubmitIdx].signalSemaphores.size(); ++i)
        {
            SEMAPHORE_SUBMIT_INFO(signalingSema);
            signalingSema.semaphore = commands[cmdSubmitIdx].signalSemaphores[i].semaphore.reference<VulkanSemaphore>()->semaphore;
            signalingSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(commands[cmdSubmitIdx].signalSemaphores[i].stages);
            signalingSemaphores.emplace_back(std::move(signalingSema));
        }
        for (int32 i = 0; i < commands[cmdSubmitIdx].signalTimelines.size(); ++i)
        {
            SEMAPHORE_SUBMIT_INFO(signalingSema);
            signalingSema.semaphore = commands[cmdSubmitIdx].signalTimelines[i].semaphore.reference<VulkanTimelineSemaphore>()->semaphore;
            signalingSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(commands[cmdSubmitIdx].signalTimelines[i].stages);
            signalingSema.value = commands[cmdSubmitIdx].signalTimelines[i].value;
            signalingSemaphores.emplace_back(std::move(signalingSema));
        }
        if (bHasNonTemp)
        {
            // Add an Time line semaphore for manager tracking
            TimelineSemaphoreRef submitSemaphore = graphicsHelper->createTimelineSemaphore(
                graphicsInstance, (TCHAR("AdvancedSubmitTSema_") + String::toString(cmdSubmitIdx)).c_str()
                );
            submitSemaphore->init();
            managerTSemaphores[cmdSubmitIdx] = submitSemaphore;

            SEMAPHORE_SUBMIT_INFO(signalingSema);
            signalingSema.semaphore = submitSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
            signalingSema.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            signalingSema.value = 1;
            signalingSemaphores.emplace_back(std::move(signalingSema));
        }

        SUBMIT_INFO2(submitInfo);
        submitInfo.commandBufferInfoCount = uint32(cmdBuffers.size());
        submitInfo.pCommandBufferInfos = cmdBuffers.data();
        submitInfo.signalSemaphoreInfoCount = uint32(signalingSemaphores.size());
        submitInfo.pSignalSemaphoreInfos = signalingSemaphores.data();
        submitInfo.waitSemaphoreInfoCount = uint32(waitOnSemaphores.size());
        submitInfo.pWaitSemaphoreInfos = waitOnSemaphores.data();

        allSubmitInfo.emplace_back(std::move(submitInfo));
    }

    if (!cmdsCompleteFence.isValid())
    {
        cmdsCompleteFence = graphicsHelper->createFence(graphicsInstance, TCHAR("AdvancedSubmitFence"));
        cmdsCompleteFence->init();
    }

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit2KHR(
        vQueue, uint32(allSubmitInfo.size()), allSubmitInfo.data(), cmdsCompleteFence.reference<VulkanFence>()->fence
    );
    fatalAssertf(result == VK_SUCCESS, "Failed submitting command to queue %s(result: %d)", queueRes->getResourceName().getChar(), result);

    for (uint32 cmdSubmitIdx = 0; cmdSubmitIdx != commands.size(); ++cmdSubmitIdx)
    {
        const CommandSubmitInfo &command = commands[cmdSubmitIdx];

        bool bAnyNonTemp = false;
        int32 index = int32(cmdsSyncInfo.get());
        VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[index];
        for (const GraphicsResource *cmdBuffer : command.cmdBuffers)
        {
            if (!static_cast<const VulkanCommandBuffer *>(cmdBuffer)->bIsTempBuffer)
            {
                bAnyNonTemp = true;
                commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
                commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
            }
        }
        if (bAnyNonTemp)
        {
            debugAssert(managerTSemaphores[cmdSubmitIdx].isValid());
            syncInfo.signalingSemaphore = managerTSemaphores[cmdSubmitIdx];
            syncInfo.completeFence = cmdsCompleteFence;
            syncInfo.refCount = uint32(command.cmdBuffers.size());
        }
        else
        {
            cmdsSyncInfo.reset(index);
        }
    }
}

void VulkanCmdBufferManager::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &command, FenceRef cmdsCompleteFence)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();
    QueueResourceBase *queueRes = nullptr;

    TimelineSemaphoreRef managerTSemaphore;

    std::vector<VkCommandBufferSubmitInfo> cmdBuffers;
    cmdBuffers.reserve(command.cmdBuffers.size());
    std::vector<VkSemaphoreSubmitInfo> waitOnSemaphores;
    waitOnSemaphores.reserve(command.waitOn.size() + command.waitOnTimelines.size());
    std::vector<VkSemaphoreSubmitInfo> signalingSemaphores;
    signalingSemaphores.reserve(command.signalSemaphores.size() + command.signalTimelines.size());

    bool bHasNonTemp = false;
    for (int32 i = 0; i < command.cmdBuffers.size(); ++i)
    {
        const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(command.cmdBuffers[i]);
        bHasNonTemp = bHasNonTemp || !vCmdBuffer->bIsTempBuffer;

        CMDBUFFER_SUBMIT_INFO(cmdBufferSubmitInfo);
        cmdBufferSubmitInfo.commandBuffer = vCmdBuffer->cmdBuffer;
        cmdBuffers.emplace_back(std::move(cmdBufferSubmitInfo));

        auto &cmdPool = getPool(vCmdBuffer->fromQueue);
        if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
        {
            LOG_ERROR("VulkanCommandBufferManager", "Buffers from different queues cannot be submitted together");
            return;
        }
        queueRes = cmdPool.cmdPoolInfo.queueResource;
    }
    if (queueRes == nullptr)
    {
        LOG_ERROR("VulkanCommandBufferManager", "Cannot submit as there is no queue found for command buffers");
        return;
    }

    for (int32 i = 0; i < command.waitOn.size(); ++i)
    {
        SEMAPHORE_SUBMIT_INFO(waitOnSema);
        waitOnSema.semaphore = command.waitOn[i].semaphore.reference<VulkanSemaphore>()->semaphore;
        waitOnSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(command.waitOn[i].stages);
        waitOnSemaphores.emplace_back(std::move(waitOnSema));
    }
    for (int32 i = 0; i < command.waitOnTimelines.size(); ++i)
    {
        SEMAPHORE_SUBMIT_INFO(waitOnSema);
        waitOnSema.semaphore = command.waitOnTimelines[i].semaphore.reference<VulkanTimelineSemaphore>()->semaphore;
        waitOnSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(command.waitOnTimelines[i].stages);
        waitOnSema.value = command.waitOnTimelines[i].value;
        waitOnSemaphores.emplace_back(std::move(waitOnSema));
    }
    for (int32 i = 0; i < command.signalSemaphores.size(); ++i)
    {
        SEMAPHORE_SUBMIT_INFO(signalingSema);
        signalingSema.semaphore = command.signalSemaphores[i].semaphore.reference<VulkanSemaphore>()->semaphore;
        signalingSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(command.signalSemaphores[i].stages);
        signalingSemaphores.emplace_back(std::move(signalingSema));
    }
    for (int32 i = 0; i < command.signalTimelines.size(); ++i)
    {
        SEMAPHORE_SUBMIT_INFO(signalingSema);
        signalingSema.semaphore = command.signalTimelines[i].semaphore.reference<VulkanTimelineSemaphore>()->semaphore;
        signalingSema.stageMask = EngineToVulkanAPI::vulkanPipelineStageFlags(command.signalTimelines[i].stages);
        signalingSema.value = command.signalTimelines[i].value;
        signalingSemaphores.emplace_back(std::move(signalingSema));
    }
    if (bHasNonTemp)
    {
        // Add an Time line semaphore for manager tracking
        managerTSemaphore = graphicsHelper->createTimelineSemaphore(graphicsInstance, TCHAR("AdvancedSubmitTSema") );
        managerTSemaphore->init();

        SEMAPHORE_SUBMIT_INFO(signalingSema);
        signalingSema.semaphore = managerTSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
        signalingSema.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        signalingSema.value = 1;
        signalingSemaphores.emplace_back(std::move(signalingSema));
    }

    SUBMIT_INFO2(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferInfoCount = uint32(cmdBuffers.size());
    cmdSubmitInfo.pCommandBufferInfos = cmdBuffers.data();
    cmdSubmitInfo.signalSemaphoreInfoCount = uint32(signalingSemaphores.size());
    cmdSubmitInfo.pSignalSemaphoreInfos = signalingSemaphores.data();
    cmdSubmitInfo.waitSemaphoreInfoCount = uint32(waitOnSemaphores.size());
    cmdSubmitInfo.pWaitSemaphoreInfos = waitOnSemaphores.data();

    if (!cmdsCompleteFence.isValid())
    {
        cmdsCompleteFence = graphicsHelper->createFence(graphicsInstance, TCHAR("AdvancedSubmitBatched"));
        cmdsCompleteFence->init();
    }
    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit2KHR(
        vQueue, 1, &cmdSubmitInfo, (cmdsCompleteFence.isValid() ? cmdsCompleteFence.reference<VulkanFence>()->fence : nullptr)
    );
    fatalAssertf(result == VK_SUCCESS, "Failed submitting command to queue %s(result: %d)", queueRes->getResourceName().getChar(), result);

    bool bAnyNonTemp = false;
    int32 index = int32(cmdsSyncInfo.get());
    VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[index];
    for (const GraphicsResource *cmdBuffer : command.cmdBuffers)
    {
        if (!static_cast<const VulkanCommandBuffer *>(cmdBuffer)->bIsTempBuffer)
        {
            bAnyNonTemp = true;
            commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
            commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
        }
    }
    if (bAnyNonTemp)
    {
        debugAssert(managerTSemaphore.isValid());
        syncInfo.signalingSemaphore = managerTSemaphore;
        syncInfo.completeFence = cmdsCompleteFence;
        syncInfo.refCount = uint32(command.cmdBuffers.size());
    }
    else
    {
        cmdsSyncInfo.reset(index);
    }
}

void VulkanCmdBufferManager::submitCmds(
    EQueuePriority::Enum priority, ArrayView<const CommandSubmitInfo2> commands, VulkanResourcesTracker *resourceTracker
)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();
    QueueResourceBase *queueRes = nullptr;

    std::vector<std::vector<VkCommandBufferSubmitInfo>> allCmdBuffers(commands.size());
    std::vector<std::vector<VkSemaphoreSubmitInfo>> allWaitOnSemaphores(commands.size());
    std::vector<std::vector<VkSemaphoreSubmitInfo>> allSignalingSemaphores(commands.size());
    std::vector<VkSubmitInfo2> allSubmitInfo;
    allSubmitInfo.reserve(commands.size());

    // fill command buffer vector, all wait informations and make sure there is no error so far
    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        std::vector<VkCommandBufferSubmitInfo> &cmdBuffers = allCmdBuffers[cmdSubmitIdx];
        cmdBuffers.reserve(commands[cmdSubmitIdx].cmdBuffers.size());
        std::vector<VkSemaphoreSubmitInfo> &waitOnSemaphores = allWaitOnSemaphores[cmdSubmitIdx];

        for (int32 i = 0; i < commands[cmdSubmitIdx].cmdBuffers.size(); ++i)
        {
            const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(commands[cmdSubmitIdx].cmdBuffers[i]);
            if (vCmdBuffer->bIsTempBuffer)
            {
                LOG_ERROR(
                    "VulkanCommandBufferManager",
                    "Reuse/One time record buffers are required to use advanced submit function, \"%s\" is temporary cmd buffer",
                    vCmdBuffer->getResourceName().getChar()
                );
                return;
            }

            CMDBUFFER_SUBMIT_INFO(cmdBufferSubmitInfo);
            cmdBufferSubmitInfo.commandBuffer = vCmdBuffer->cmdBuffer;
            cmdBuffers.emplace_back(std::move(cmdBufferSubmitInfo));

            auto &cmdPool = getPool(vCmdBuffer->fromQueue);
            if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
            {
                LOG_ERROR("VulkanCommandBufferManager", "Buffers from different queues cannot be submitted together");
                return;
            }
            queueRes = cmdPool.cmdPoolInfo.queueResource;

            // Resource tracked waits
            const std::vector<VulkanResourcesTracker::CommandResUsageInfo> *resWaits = resourceTracker->getCmdBufferDeps(vCmdBuffer);
            if (resWaits)
            {
                for (const VulkanResourcesTracker::CommandResUsageInfo &waitOn : *resWaits)
                {
                    auto cmdBufferItr = commandBuffers.find(waitOn.cmdBuffer->getResourceName());
                    if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
                    {
                        LOG_ERROR(
                            "VulkanCommandBufferManager", "Waiting on cmd buffer[%s] is invalid or not submitted",
                            waitOn.cmdBuffer->getResourceName().getChar()
                        );
                        return;
                    }

                    const VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
                    // Do not add if completed already
                    if (syncInfo.completeFence->isSignaled())
                    {
                        continue;
                    }
                    SEMAPHORE_SUBMIT_INFO(waitOnSema);
                    waitOnSema.semaphore = syncInfo.signalingSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
                    waitOnSema.stageMask = waitOn.usedDstStages;
                    waitOnSema.value = 1;
                    waitOnSemaphores.emplace_back(std::move(waitOnSema));
                }
            }
        }
        if (queueRes == nullptr)
        {
            LOG_ERROR("VulkanCommandBufferManager", "Cannot submit as there is no queue found for command buffers");
            return;
        }

        // Manual waits
        for (const GraphicsResource *waitOn : commands[cmdSubmitIdx].waitOnCmdBuffers)
        {
            auto cmdBufferItr = commandBuffers.find(waitOn->getResourceName());
            if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
            {
                LOG_ERROR(
                    "VulkanCommandBufferManager", "Waiting on cmd buffer[%s] is invalid or not submitted", waitOn->getResourceName().getChar()
                );
                return;
            }

            const VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
            SEMAPHORE_SUBMIT_INFO(waitOnSema);
            waitOnSema.semaphore = syncInfo.signalingSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
            waitOnSema.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            waitOnSema.value = 1;
            waitOnSemaphores.emplace_back(std::move(waitOnSema));
        }

        SUBMIT_INFO2(cmdSubmitInfo);
        cmdSubmitInfo.commandBufferInfoCount = uint32(cmdBuffers.size());
        cmdSubmitInfo.pCommandBufferInfos = cmdBuffers.data();
        cmdSubmitInfo.waitSemaphoreInfoCount = uint32(waitOnSemaphores.size());
        cmdSubmitInfo.pWaitSemaphoreInfos = waitOnSemaphores.data();
        allSubmitInfo.emplace_back(std::move(cmdSubmitInfo));
    }

    FenceRef cmdsCompleteFence = graphicsHelper->createFence(graphicsInstance, TCHAR("SubmitBatched"));
    cmdsCompleteFence->init();

    // Fill all signaling semaphores, Set cmd states
    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        int32 index = int32(cmdsSyncInfo.get());
        VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[index];
        syncInfo.completeFence = cmdsCompleteFence;
        syncInfo.refCount = uint32(commands[cmdSubmitIdx].cmdBuffers.size());

        for (const GraphicsResource *cmdBuffer : commands[cmdSubmitIdx].cmdBuffers)
        {
            commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
            commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
        }

        // Add an Time line semaphore for manager tracking
        syncInfo.signalingSemaphore
            = graphicsHelper->createTimelineSemaphore(graphicsInstance, (TCHAR("SubmitTSema_") + String::toString(cmdSubmitIdx)).c_str());
        syncInfo.signalingSemaphore->init();

        std::vector<VkSemaphoreSubmitInfo> &signalingSemaphores = allSignalingSemaphores[cmdSubmitIdx];
        SEMAPHORE_SUBMIT_INFO(signalingSema);
        signalingSema.semaphore = syncInfo.signalingSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
        signalingSema.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        signalingSema.value = 1;
        signalingSemaphores.emplace_back(std::move(signalingSema));

        allSubmitInfo[cmdSubmitIdx].signalSemaphoreInfoCount = uint32(signalingSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pSignalSemaphoreInfos = signalingSemaphores.data();
    }

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit2KHR(
        vQueue, uint32(allSubmitInfo.size()), allSubmitInfo.data(),
        (cmdsCompleteFence.isValid() ? cmdsCompleteFence.reference<VulkanFence>()->fence : nullptr)
    );
    fatalAssertf(result == VK_SUCCESS, "Failed submitting command to queue %s(result: %d)", queueRes->getResourceName().getChar(), result);
}

void VulkanCmdBufferManager::submitCmd(
    EQueuePriority::Enum priority, const CommandSubmitInfo2 &command, VulkanResourcesTracker *resourceTracker
)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();

    QueueResourceBase *queueRes = nullptr;

    std::vector<VkCommandBufferSubmitInfo> cmdBuffers;
    cmdBuffers.reserve(command.cmdBuffers.size());
    std::vector<VkSemaphoreSubmitInfo> waitOnSemaphores;
    std::vector<VkSemaphoreSubmitInfo> signalingSemaphores;

    for (int32 i = 0; i < command.cmdBuffers.size(); ++i)
    {
        const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(command.cmdBuffers[i]);
        if (vCmdBuffer->bIsTempBuffer)
        {
            LOG_ERROR(
                "VulkanCommandBufferManager", "Temporary buffers[%s] are required to use advanced submit function",
                vCmdBuffer->getResourceName().getChar()
            );
            return;
        }

        CMDBUFFER_SUBMIT_INFO(cmdBufferSubmitInfo);
        cmdBufferSubmitInfo.commandBuffer = vCmdBuffer->cmdBuffer;
        cmdBuffers.emplace_back(std::move(cmdBufferSubmitInfo));

        auto &cmdPool = getPool(vCmdBuffer->fromQueue);
        if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
        {
            LOG_ERROR("VulkanCommandBufferManager", "Buffers from different queues cannot be submitted together");
            return;
        }
        queueRes = cmdPool.cmdPoolInfo.queueResource;

        // Resource tracked waits
        const std::vector<VulkanResourcesTracker::CommandResUsageInfo> *resWaits = resourceTracker->getCmdBufferDeps(vCmdBuffer);
        if (resWaits)
        {
            for (const VulkanResourcesTracker::CommandResUsageInfo &waitOn : *resWaits)
            {
                auto cmdBufferItr = commandBuffers.find(waitOn.cmdBuffer->getResourceName());
                if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
                {
                    LOG_ERROR(
                        "VulkanCommandBufferManager", "Waiting on cmd buffer[%s] is invalid or not submitted",
                        waitOn.cmdBuffer->getResourceName().getChar()
                    );
                    return;
                }

                const VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
                // Do not add if completed already
                if (syncInfo.completeFence->isSignaled())
                {
                    continue;
                }
                SEMAPHORE_SUBMIT_INFO(waitOnSema);
                waitOnSema.semaphore = syncInfo.signalingSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
                waitOnSema.stageMask = waitOn.usedDstStages;
                waitOnSema.value = 1;
                waitOnSemaphores.emplace_back(std::move(waitOnSema));
            }
        }
    }
    if (queueRes == nullptr)
    {
        LOG_ERROR("VulkanCommandBufferManager", "Cannot submit as there is no queue found for command buffers");
        return;
    }

    for (const GraphicsResource *waitOn : command.waitOnCmdBuffers)
    {
        auto cmdBufferItr = commandBuffers.find(waitOn->getResourceName());
        if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
        {
            LOG_ERROR(
                "VulkanCommandBufferManager", "Waiting on cmd buffer[%s] is invalid or not submitted", waitOn->getResourceName().getChar()
            );
            return;
        }

        const VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
        SEMAPHORE_SUBMIT_INFO(waitOnSema);
        waitOnSema.semaphore = syncInfo.signalingSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
        waitOnSema.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        waitOnSema.value = 1;
        waitOnSemaphores.emplace_back(std::move(waitOnSema));
    }

    FenceRef cmdsCompleteFence = graphicsHelper->createFence(graphicsInstance, TCHAR("SubmitBatched"));
    cmdsCompleteFence->init();

    int32 index = int32(cmdsSyncInfo.get());
    VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[index];
    syncInfo.completeFence = cmdsCompleteFence;
    syncInfo.refCount = uint32(command.cmdBuffers.size());

    for (const GraphicsResource *cmdBuffer : command.cmdBuffers)
    {
        commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
        commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
    }

    // Add an Time line semaphore for manager tracking
    syncInfo.signalingSemaphore = graphicsHelper->createTimelineSemaphore(graphicsInstance, TCHAR("SubmitTSema"));
    syncInfo.signalingSemaphore->init();

    SEMAPHORE_SUBMIT_INFO(signalingSema);
    signalingSema.semaphore = syncInfo.signalingSemaphore.reference<VulkanTimelineSemaphore>()->semaphore;
    signalingSema.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    signalingSema.value = 1;
    signalingSemaphores.emplace_back(std::move(signalingSema));

    SUBMIT_INFO2(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferInfoCount = uint32(cmdBuffers.size());
    cmdSubmitInfo.pCommandBufferInfos = cmdBuffers.data();
    cmdSubmitInfo.signalSemaphoreInfoCount = uint32(signalingSemaphores.size());
    cmdSubmitInfo.pSignalSemaphoreInfos = signalingSemaphores.data();
    cmdSubmitInfo.waitSemaphoreInfoCount = uint32(waitOnSemaphores.size());
    cmdSubmitInfo.pWaitSemaphoreInfos = waitOnSemaphores.data();

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit2KHR(
        vQueue, 1, &cmdSubmitInfo, (cmdsCompleteFence.isValid() ? cmdsCompleteFence.reference<VulkanFence>()->fence : nullptr)
    );
    fatalAssertf(result == VK_SUCCESS, "Failed submitting command to queue %s(result: %d)", queueRes->getResourceName().getChar(), result);
}

void VulkanCmdBufferManager::createPools()
{
    VkDevice logicalDevice = VulkanGraphicsHelper::getDevice(vDevice);
    if (vDevice->getComputeQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Compute> *queue
            = static_cast<VulkanQueueResource<EQueueFunction::Compute> *>(vDevice->getComputeQueue());
        VulkanCommandPool &pool = pools[EQueueFunction::Compute];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Compute };
        pool.init();
    }

    if (vDevice->getGraphicsQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Graphics> *queue
            = static_cast<VulkanQueueResource<EQueueFunction::Graphics> *>(vDevice->getGraphicsQueue());
        VulkanCommandPool &pool = pools[EQueueFunction::Graphics];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Graphics };
        pool.init();
    }

    if (vDevice->getTransferQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Transfer> *queue
            = static_cast<VulkanQueueResource<EQueueFunction::Transfer> *>(vDevice->getTransferQueue());
        VulkanCommandPool &pool = pools[EQueueFunction::Transfer];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Transfer };
        pool.init();
    }

    if (vDevice->getGenericQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Generic> *queue
            = static_cast<VulkanQueueResource<EQueueFunction::Generic> *>(vDevice->getGenericQueue());
        VulkanCommandPool &pool = pools[EQueueFunction::Generic];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Generic };
        pool.init();
        genericPool = &pool;
    }

    if (getQueue<EQueueFunction::Present>(vDevice) != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Present> *queue = getQueue<EQueueFunction::Present>(vDevice);
        VulkanCommandPool &pool = pools[EQueueFunction::Present];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Present };
        pool.init();
    }
}

VulkanCommandPool &VulkanCmdBufferManager::getPool(EQueueFunction forQueue)
{
    auto cmdPoolItr = pools.find(forQueue);
    if (cmdPoolItr == pools.end())
    {
        fatalAssertf(genericPool, "Generic pool must be available");
        return *genericPool;
    }
    else
    {
        return cmdPoolItr->second;
    }
}

VkQueue VulkanCmdBufferManager::getVkQueue(EQueuePriority::Enum priority, QueueResourceBase *queueRes)
{
    switch (priority)
    {
    case EQueuePriority::Low:
        return VulkanQueueResourceInvoker::invoke<VkQueue, GetQueueOfPriorityLow>(queueRes);
        break;
    case EQueuePriority::Medium:
        return VulkanQueueResourceInvoker::invoke<VkQueue, GetQueueOfPriorityMedium>(queueRes);
        break;
    case EQueuePriority::High:
        return VulkanQueueResourceInvoker::invoke<VkQueue, GetQueueOfPriorityHigh>(queueRes);
        break;
    case EQueuePriority::SuperHigh:
        return VulkanQueueResourceInvoker::invoke<VkQueue, GetQueueOfPrioritySuperHigh>(queueRes);
        break;
    default:
        return VulkanQueueResourceInvoker::invoke<VkQueue, GetQueueOfPriorityMedium>(queueRes);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////
//// VulkanResourcesTracker Implementations
///////////////////////////////////////////////////////////////////////////

const std::vector<VulkanResourcesTracker::CommandResUsageInfo> *VulkanResourcesTracker::getCmdBufferDeps(const GraphicsResource *cmdBuffer
) const
{
    CmdWaitInfoMap::const_iterator itr = cmdWaitInfo.find(cmdBuffer);
    if (itr != cmdWaitInfo.cend())
    {
        return &itr->second;
    }
    return nullptr;
}

std::vector<const GraphicsResource *> VulkanResourcesTracker::getCmdBufferResourceDeps(const MemoryResourceRef &resource) const
{
    std::vector<const GraphicsResource *> retVal;

    auto resourceAccessorItr = resourcesAccessors.find(resource);
    if (resourceAccessorItr != resourcesAccessors.cend())
    {
        if (resourceAccessorItr->second.lastWrite)
        {
            retVal.emplace_back(resourceAccessorItr->second.lastWrite);
        }
        retVal.insert(retVal.end(), resourceAccessorItr->second.lastReadsIn.cbegin(), resourceAccessorItr->second.lastReadsIn.cend());
    }
    return retVal;
}

std::vector<const GraphicsResource *> VulkanResourcesTracker::getDependingCmdBuffers(const GraphicsResource *cmdBuffer) const
{
    std::vector<const GraphicsResource *> retVal;
    for (const std::pair<const GraphicsResource *const, std::vector<CommandResUsageInfo>> &cmdWaitOn : cmdWaitInfo)
    {
        auto cmdUsageInfoItr = std::find_if(
            cmdWaitOn.second.cbegin(), cmdWaitOn.second.cend(),
            [cmdBuffer](const CommandResUsageInfo &usageInfo)
            {
                return usageInfo.cmdBuffer == cmdBuffer;
            }
        );
        if (cmdUsageInfoItr != cmdWaitOn.second.cend())
        {
            retVal.emplace_back(cmdWaitOn.first);
        }
    }
    return retVal;
}

void VulkanResourcesTracker::clearFinishedCmd(const GraphicsResource *cmdBuffer)
{
    cmdWaitInfo.erase(cmdBuffer);

    // Remove cmdBuffer from read list and write and remove resource if no cmd buffer is related to it
    for (auto resAccessorItr = resourcesAccessors.begin(); resAccessorItr != resourcesAccessors.end();)
    {
        if (resAccessorItr->second.lastWrite == cmdBuffer)
        {
            resAccessorItr->second.lastWrite = nullptr;
        }

        SizeT erasedCount = std::erase(resAccessorItr->second.lastReadsIn, cmdBuffer);

        // if there is no last write or if there was read after write and all read is finished and empty, Then clear it
        if ((resAccessorItr->second.lastWrite == nullptr || erasedCount > 0) && resAccessorItr->second.lastReadsIn.empty())
        {
            resAccessorItr = resourcesAccessors.erase(resAccessorItr);
        }
        else
        {
            ++resAccessorItr;
        }
    }
}

void VulkanResourcesTracker::clearResource(const MemoryResourceRef &resource)
{
    resourcesAccessors.erase(resource);

    for (uint32 i = 0; i < ARRAY_LENGTH(queueTransfers); ++i)
    {
        queueTransfers[i].erase(resource.get());
    }
}

void VulkanResourcesTracker::clearUnwanted()
{
    std::unordered_set<const GraphicsResource *> memResources;
    {
        std::vector<GraphicsResource *> memRes;
        MemoryResource::staticType()->allRegisteredResources(memRes, true);
        memResources.insert(memRes.cbegin(), memRes.cend());
    }

    for (std::map<MemoryResourceRef, ResourceAccessors>::iterator itr = resourcesAccessors.begin(); itr != resourcesAccessors.end();)
    {
        // If we are the last one holding reference to a resource release it
        if (itr->first->refCount() == 1)
        {
            itr = resourcesAccessors.erase(itr);
        }
        else
        {
            // Remove duplicate reads preserving first read alone
            if (itr->second.lastReadsIn.size() > 1)
            {
                // Since we need to preserve first read alone
                const GraphicsResource *firstRead = itr->second.lastReadsIn[0];
                std::unordered_set<const GraphicsResource *> uniqueReads;
                uniqueReads.insert(firstRead);

                auto newEnd = std::remove_if(
                    itr->second.lastReadsIn.begin(), itr->second.lastReadsIn.end(),
                    [&uniqueReads](const GraphicsResource *res)
                    {
                        return !uniqueReads.insert(res).second;
                    }
                );
                itr->second.lastReadsIn.erase(newEnd, itr->second.lastReadsIn.end());
                // Restore first read
                itr->second.lastReadsIn.emplace_back(itr->second.lastReadsIn[0]);
                itr->second.lastReadsIn[0] = firstRead;
            }
            ++itr;
        }
    }

    for (uint32 i = 0; i < ARRAY_LENGTH(queueTransfers); ++i)
    {
        for (std::map<MemoryResource *, ResourceUsedQueue>::iterator itr = queueTransfers[i].begin(); itr != queueTransfers[i].end();)
        {
            if (!memResources.contains(itr->first))
            {
                itr = queueTransfers[i].erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }
    for (auto itr = resourceReleases.begin(); itr != resourceReleases.end();)
    {
        if (!memResources.contains(itr->first))
        {
            itr = resourceReleases.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::readOnlyBuffers(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    const EQueueFunction cmdBufferQ = static_cast<const VulkanCommandBuffer *>(cmdBuffer)->fromQueue;

    OptionalBarrierInfo outBarrierInfo = {};
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    if (!accessors.lastWrite)
    {
        accessors.addLastReadInCmd(cmdBuffer);
        accessors.allReadStages |= resource.second;
        accessors.lastReadStages = resource.second;

        // If nothing is found at least we might have to do queue transfers
        auto resReleasedAtQItr = resourceReleases.find(resource.first.get());
        if (resReleasedAtQItr != resourceReleases.cend())
        {
            outBarrierInfo = resReleasedAtQItr->second;
            resourceReleases.erase(resReleasedAtQItr);
        }
        return outBarrierInfo;
    }
    // Clear the last release information since we do not need it anymore, Until further release
    resourceReleases.erase(resource.first.get());

    if (accessors.lastReadsIn.empty())
    {

        if (accessors.lastWrite == cmdBuffer)
        {
            // If this is the first barrier within this command for this resource
            ResourceBarrierInfo barrier;
            barrier.accessors.lastWrite = accessors.lastWrite;
            barrier.accessors.lastWriteStage = accessors.lastWriteStage;
            barrier.resource = resource.first;

            outBarrierInfo = barrier;
        }
        else
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, resource.second });
            // if last write is not in this queue then we need barrier to do queue transfer
            if (cmdBufferQ != static_cast<const VulkanCommandBuffer *>(accessors.lastWrite)->fromQueue)
            {
                ResourceBarrierInfo barrier;
                barrier.accessors.lastWrite = accessors.lastWrite;
                barrier.accessors.lastWriteStage = accessors.lastWriteStage;
                barrier.resource = resource.first;
                outBarrierInfo = barrier;
            }
        }
    }
    else if (cmdBufferQ != static_cast<const VulkanCommandBuffer *>(accessors.lastReadsIn.back())->fromQueue)
    {
        cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastReadsIn.back(), resource.second });

        fatalAssertf(
            cmdBufferQ != static_cast<const VulkanCommandBuffer *>(accessors.lastReadsIn.back())->fromQueue,
            "This is valid usage however this case for read buffer is not supported in VulkanRenderCmdList"
        );
        ResourceBarrierInfo barrier;
        barrier.accessors.lastWrite = nullptr;
        barrier.accessors.lastWriteStage = 0;
        barrier.accessors.addLastReadInCmd(accessors.lastReadsIn.back());
        barrier.accessors.lastReadStages = accessors.lastReadStages;
        barrier.accessors.allReadStages = accessors.allReadStages;
        barrier.resource = resource.first;

        outBarrierInfo = barrier;
    }

    accessors.addLastReadInCmd(cmdBuffer);
    accessors.allReadStages |= resource.second;
    accessors.lastReadStages = resource.second;
    return outBarrierInfo;
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::readOnlyImages(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    const EQueueFunction cmdBufferQ = static_cast<const VulkanCommandBuffer *>(cmdBuffer)->fromQueue;

    OptionalBarrierInfo outBarrierInfo = {};
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    if (!accessors.lastWrite)
    {
        accessors.addLastReadInCmd(cmdBuffer);
        accessors.allReadStages |= resource.second;
        accessors.lastReadStages = resource.second;

        // If nothing is found at least we might have to do queue transfers
        auto resReleasedAtQItr = resourceReleases.find(resource.first.get());
        if (resReleasedAtQItr != resourceReleases.cend())
        {
            outBarrierInfo = resReleasedAtQItr->second;
            resourceReleases.erase(resReleasedAtQItr);
        }
        return outBarrierInfo;
    }
    // Clear the last release information since we do not need it anymore, Until further release
    resourceReleases.erase(resource.first.get());

    // If never read after last write, then layout needs transition before this read no matter write is
    // in this cmd or others
    if (accessors.lastReadsIn.empty())
    {
        ResourceBarrierInfo barrier;
        barrier.accessors.lastWrite = accessors.lastWrite;
        barrier.accessors.lastWriteStage = accessors.lastWriteStage;
        barrier.resource = resource.first;

        // Last write if not same cmd then wait on that command
        if (accessors.lastWrite && accessors.lastWrite != cmdBuffer)
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, resource.second });
        }

        outBarrierInfo = barrier;
    }
    else
    {
        cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, resource.second });
        // If layout transition is not done on this cmd buffer, then wait on it as well(So long as this is the first read in this cmdBuffer)
        if (accessors.lastReadsIn.front() != cmdBuffer && accessors.lastReadsIn.back() != cmdBuffer)
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastReadsIn.front(), resource.second });
            // If last read in cmd buffer queue is not same as current queue, we need queue transfer barrier
            if (cmdBufferQ != static_cast<const VulkanCommandBuffer *>(accessors.lastReadsIn.back())->fromQueue)
            {
                ResourceBarrierInfo barrier;
                barrier.accessors.addLastReadInCmd(accessors.lastReadsIn.back());
                barrier.accessors.lastReadStages = barrier.accessors.allReadStages = accessors.lastReadStages;
                barrier.resource = resource.first;

                outBarrierInfo = barrier;
            }
        }
    }
    accessors.addLastReadInCmd(cmdBuffer);
    accessors.allReadStages |= resource.second;
    accessors.lastReadStages = resource.second;
    return outBarrierInfo;
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::readOnlyTexels(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::readFromWriteBuffers(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::readFromWriteImages(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    return readOnlyImages(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::readFromWriteTexels(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::writeReadOnlyBuffers(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    fatalAssertf(PlatformFunctions::getSetBitCount(resource.second) == 1, "Writing to buffer in several pipeline stages is incorrect");

    const EQueueFunction cmdBufferQ = static_cast<const VulkanCommandBuffer *>(cmdBuffer)->fromQueue;

    OptionalBarrierInfo outBarrierInfo = {};
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits(resource.second);
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    // If never read or write
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;

        // If nothing is found at least we might have to do queue transfers
        auto resReleasedAtQItr = resourceReleases.find(resource.first.get());
        if (resReleasedAtQItr != resourceReleases.cend())
        {
            outBarrierInfo = resReleasedAtQItr->second;
            resourceReleases.erase(resReleasedAtQItr);
        }
        return outBarrierInfo;
    }
    // Clear the last release information since we do not need it anymore, Until further release
    resourceReleases.erase(resource.first.get());

    // If we are already reading in this cmd buffer then all other steps are already done so wait for
    // just read to finish
    if (std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) != accessors.lastReadsIn.cend())
    {
        // TODO(Jeslas): Check if cmd not waiting on other reads is an issue here?
        ResourceBarrierInfo barrier;
        barrier.accessors.addLastReadInCmd(cmdBuffer);
        barrier.resource = resource.first;
        barrier.accessors.allReadStages = accessors.allReadStages;
        barrier.accessors.lastReadStages = accessors.lastReadStages;

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        accessors.lastReadsIn.clear();
        accessors.lastReadStages = 0;
        accessors.allReadStages = 0;

        outBarrierInfo = barrier;
        return outBarrierInfo;
    }

    if (!accessors.lastReadsIn.empty()) // If not empty then there is other cmds that are reading so wait for those cmds
    {
        const GraphicsResource *readInDiffQ = nullptr;
        for (const GraphicsResource *readInCmdBuffer : accessors.lastReadsIn)
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ readInCmdBuffer, resource.second });
            if (cmdBufferQ != static_cast<const VulkanCommandBuffer *>(readInCmdBuffer)->fromQueue)
            {
                // It is okay as it is fine to wait on last submitted read queue
                readInDiffQ = readInCmdBuffer;
            }
        }

        // we do not have to wait for last write as reads already do that, Unless queue changes
        if (readInDiffQ)
        {
            ResourceBarrierInfo barrier;
            barrier.accessors.addLastReadInCmd(readInDiffQ);
            barrier.resource = resource.first;
            barrier.accessors.allReadStages = accessors.allReadStages;
            barrier.accessors.lastReadStages = accessors.lastReadStages;

            outBarrierInfo = barrier;
        }
        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        accessors.lastReadsIn.clear();
        accessors.lastReadStages = 0;
        accessors.allReadStages = 0;
        return outBarrierInfo;
    }

    if (accessors.lastWrite)
    {
        // If lastWrite is not in this queue then transfer has to happen
        bool bApplyBarrier = true;
        if (accessors.lastWrite != cmdBuffer)
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, resource.second });
            bApplyBarrier = cmdBufferQ != static_cast<const VulkanCommandBuffer *>(accessors.lastWrite)->fromQueue;
        }

        if (bApplyBarrier)
        {
            ResourceBarrierInfo barrier;
            barrier.accessors.lastWrite = accessors.lastWrite;
            barrier.accessors.lastWriteStage = accessors.lastWriteStage;
            barrier.resource = resource.first;

            outBarrierInfo = barrier;
        }
    }
    accessors.lastWrite = cmdBuffer;
    accessors.lastWriteStage = stageFlag;
    return outBarrierInfo;
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::writeReadOnlyImages(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    fatalAssertf(PlatformFunctions::getSetBitCount(resource.second) == 1, "Writing to image in several pipeline stages is incorrect");

    OptionalBarrierInfo outBarrierInfo = {};
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits(resource.second);
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    // If never read or write
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;

        // If nothing is found at least we might have to do queue transfers
        auto resReleasedAtQItr = resourceReleases.find(resource.first.get());
        if (resReleasedAtQItr != resourceReleases.cend())
        {
            outBarrierInfo = resReleasedAtQItr->second;
            resourceReleases.erase(resReleasedAtQItr);
        }
        else
        {
            // Since image layout for Read/writes img depends on caller, use empty read write case to handle it
            outBarrierInfo = ResourceBarrierInfo{};
        }

        return outBarrierInfo;
    }
    // Clear the last release information since we do not need it anymore, Until further release
    resourceReleases.erase(resource.first.get());

    // If we are already reading in this cmd buffer then all other steps are already done so wait for
    // just read to finish
    if (std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) != accessors.lastReadsIn.cend())
    {
        // Since same command buffer we need to write after waiting for read
        ResourceBarrierInfo barrier;
        barrier.accessors.addLastReadInCmd(cmdBuffer);
        barrier.resource = resource.first;
        barrier.accessors.allReadStages = accessors.allReadStages;
        barrier.accessors.lastReadStages = accessors.lastReadStages;

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        accessors.lastReadsIn.clear();
        accessors.allReadStages = accessors.lastReadStages = 0;

        outBarrierInfo = barrier;
        return outBarrierInfo;
    }

    // If not empty then there is other cmds that are reading so wait for those cmds, and transfer layout
    if (!accessors.lastReadsIn.empty())
    {
        for (const GraphicsResource *readInCmdBuffer : accessors.lastReadsIn)
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ readInCmdBuffer, resource.second });

        ResourceBarrierInfo barrier;
        barrier.accessors.lastReadsIn = accessors.lastReadsIn;
        barrier.resource = resource.first;
        barrier.accessors.allReadStages = accessors.allReadStages;
        barrier.accessors.lastReadStages = accessors.lastReadStages;
        // we do not have to wait for last write as reads already do that
        outBarrierInfo = barrier;

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        accessors.lastReadsIn.clear();
        accessors.allReadStages = accessors.lastReadStages = 0;

        return outBarrierInfo;
    }

    if (accessors.lastWrite)
    {
        if (accessors.lastWrite != cmdBuffer)
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, resource.second });
        }
        else
        {
            ResourceBarrierInfo barrier;
            barrier.accessors.lastWrite = accessors.lastWrite;
            barrier.accessors.lastWriteStage = accessors.lastWriteStage;
            barrier.resource = resource.first;

            outBarrierInfo = barrier;
        }
    }
    accessors.lastWrite = cmdBuffer;
    accessors.lastWriteStage = stageFlag;
    return outBarrierInfo;
}

VulkanResourcesTracker::OptionalBarrierInfo VulkanResourcesTracker::writeReadOnlyTexels(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource
)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo
    VulkanResourcesTracker::writeBuffers(const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo
    VulkanResourcesTracker::writeImages(const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource)
{
    return writeReadOnlyImages(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo
    VulkanResourcesTracker::writeTexels(const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags2> &resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

VulkanResourcesTracker::OptionalBarrierInfo
    VulkanResourcesTracker::imageToGeneralLayout(const GraphicsResource *, const ImageResourceRef &resource)
{
    OptionalBarrierInfo outBarrierInfo = {};

    auto accessorsItr = resourcesAccessors.find(resource);
    if (accessorsItr != resourcesAccessors.end())
    {
        if (accessorsItr->second.lastWrite || !accessorsItr->second.lastReadsIn.empty())
        {
            ResourceBarrierInfo barrier;
            barrier.accessors = accessorsItr->second;
            barrier.resource = resource;

            outBarrierInfo = barrier;

            // Clear the last release information since we do not need it anymore, Until further release
            resourceReleases.erase(resource.get());
        }
        else
        {
            // If nothing is found at least we might have to do queue transfers
            auto resReleasedAtQItr = resourceReleases.find(resource.get());
            if (resReleasedAtQItr != resourceReleases.cend())
            {
                outBarrierInfo = resReleasedAtQItr->second;
                resourceReleases.erase(resReleasedAtQItr);
            }
        }
        accessorsItr->second.allReadStages = accessorsItr->second.lastReadStages = 0;
        accessorsItr->second.lastReadsIn.clear();
        accessorsItr->second.lastWrite = nullptr;
    }

    return outBarrierInfo;
}

VulkanResourcesTracker::OptionalBarrierInfo
    VulkanResourcesTracker::colorAttachmentWrite(const GraphicsResource *cmdBuffer, const ImageResourceRef &resource)
{
    OptionalBarrierInfo outBarrierInfo = {};
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ResourceAccessors &accessors = resourcesAccessors[resource];

    // If never read or write, no need to do any transition unless we are loading in render pass
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;

        // If nothing is found at least we might have to do queue transfers
        auto resReleasedAtQItr = resourceReleases.find(resource.get());
        if (resReleasedAtQItr != resourceReleases.cend())
        {
            outBarrierInfo = resReleasedAtQItr->second;
            resourceReleases.erase(resReleasedAtQItr);
        }
        return outBarrierInfo;
    }
    // Clear the last release information since we do not need it anymore, Until further release
    resourceReleases.erase(resource.get());

    // If not read in same cmd buffer then there is other cmds that are reading so wait for those cmds,
    // Transition is not necessary as load/clear either way layout will be compatible
    if (!accessors.lastReadsIn.empty()
        && std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) == accessors.lastReadsIn.cend())
    {
        for (const GraphicsResource *readInCmdBuffer : accessors.lastReadsIn)
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ readInCmdBuffer, VkPipelineStageFlags2(stageFlag) });

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        accessors.lastReadsIn.clear();
        accessors.allReadStages = accessors.lastReadStages = 0;

        return outBarrierInfo;
    }

    // If last write is not in this cmd buffer then we just wait on that cmd buffer
    // Transition is not necessary as load/clear either way layout will be compatible
    if (accessors.lastWrite && accessors.lastWrite != cmdBuffer)
    {
        cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, VkPipelineStageFlags2(stageFlag) });
    }
    accessors.lastWrite = cmdBuffer;
    accessors.lastWriteStage = stageFlag;
    return outBarrierInfo;
}

void VulkanResourcesTracker::addResourceToQTransfer(
    EQueueFunction queueType, const MemoryResourceRef &resource, VkPipelineStageFlags2 usedInStages, VkAccessFlags2 accessFlags,
    VkImageLayout imageLayout, bool bReset
)
{
    std::map<MemoryResource *, ResourceUsedQueue> &queueReleaseInfo = queueTransfers[queueToQTransferIdx(queueType)];
    ResourceUsedQueue &qTransferInfo = queueReleaseInfo[resource.get()];
    qTransferInfo.srcLayout = imageLayout;
    if (bReset)
    {
        qTransferInfo.srcStages = usedInStages;
        qTransferInfo.srcAccessMask = accessFlags;
    }
    else
    {
        qTransferInfo.srcStages |= usedInStages;
        qTransferInfo.srcAccessMask |= accessFlags;
    }
}

void VulkanResourcesTracker::addResourceToQTransfer(
    EQueueFunction queueType, const MemoryResourceRef &resource, VkPipelineStageFlags2 usedInStages, VkAccessFlags2 accessFlags, bool bReset
)
{
    std::map<MemoryResource *, ResourceUsedQueue> &queueReleaseInfo = queueTransfers[queueToQTransferIdx(queueType)];
    ResourceUsedQueue &qTransferInfo = queueReleaseInfo[resource.get()];
    if (bReset)
    {
        qTransferInfo.srcStages = usedInStages;
        qTransferInfo.srcAccessMask = accessFlags;
    }
    else
    {
        qTransferInfo.srcStages |= usedInStages;
        qTransferInfo.srcAccessMask |= accessFlags;
    }
}

std::map<MemoryResource *, VulkanResourcesTracker::ResourceUsedQueue> VulkanResourcesTracker::getReleasesFromQueue(EQueueFunction queueType)
{
    std::map<MemoryResource *, ResourceUsedQueue> &queueReleaseInfo = queueTransfers[queueToQTransferIdx(queueType)];
    return std::move(queueReleaseInfo);
}

void VulkanResourcesTracker::releaseResourceAt(
    EQueueFunction queueType, const MemoryResourceRef &resource, VkPipelineStageFlags2 usedInStages, VkAccessFlags2 accessFlags
)
{
    ResourceReleasedFromQueue &releaseAtQ = resourceReleases[resource.get()];
    releaseAtQ.lastReleasedQ = queueType;
    releaseAtQ.srcStages = usedInStages;
    releaseAtQ.srcAccessMask = accessFlags;
}

void VulkanResourcesTracker::releaseResourceAt(
    EQueueFunction queueType, const MemoryResourceRef &resource, VkPipelineStageFlags2 usedInStages, VkAccessFlags2 accessFlags,
    VkImageLayout imageLayout
)
{
    ResourceReleasedFromQueue &releaseAtQ = resourceReleases[resource.get()];
    releaseAtQ.lastReleasedQ = queueType;
    releaseAtQ.srcStages = usedInStages;
    releaseAtQ.srcAccessMask = accessFlags;
    releaseAtQ.srcLayout = imageLayout;
}
