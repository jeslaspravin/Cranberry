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

#include <optional>
#include <unordered_set>

#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
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

VkCommandBuffer VulkanGraphicsHelper::getRawCmdBuffer(class IGraphicsInstance *graphicsInstance, const GraphicsResource *cmdBuffer)
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
    const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(cmdBuffer);

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

uint32 VulkanCmdBufferManager::getQueueFamilyIdx(EQueueFunction queue) const { return pools.at(queue).cmdPoolInfo.vulkanQueueIndex; }

uint32 VulkanCmdBufferManager::getQueueFamilyIdx(const GraphicsResource *cmdBuffer) const
{
    return getQueueFamilyIdx(static_cast<const VulkanCommandBuffer *>(cmdBuffer)->fromQueue);
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

SemaphoreRef VulkanCmdBufferManager::cmdSignalSemaphore(const GraphicsResource *cmdBuffer) const
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

void VulkanCmdBufferManager::submitCmds(
    EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo> &commands, FenceRef cmdsCompleteFence
)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();
    QueueResourceBase *queueRes = nullptr;

    std::vector<std::vector<VkCommandBuffer>> allCmdBuffers(commands.size());
    std::vector<std::vector<VkSemaphore>> allWaitOnSemaphores(commands.size());
    std::vector<std::vector<VkPipelineStageFlags>> allWaitingStages(commands.size());
    std::vector<std::vector<VkSemaphore>> allSignallingSemaphores(commands.size());
    std::vector<VkSubmitInfo> allSubmitInfo(commands.size());

    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        std::vector<VkCommandBuffer> &cmdBuffers = allCmdBuffers[cmdSubmitIdx];
        cmdBuffers.resize(commands[cmdSubmitIdx].cmdBuffers.size());
        std::vector<VkSemaphore> &waitOnSemaphores = allWaitOnSemaphores[cmdSubmitIdx];
        waitOnSemaphores.resize(commands[cmdSubmitIdx].waitOn.size());
        std::vector<VkPipelineStageFlags> &waitingStages = allWaitingStages[cmdSubmitIdx];
        waitingStages.resize(waitOnSemaphores.size());
        std::vector<VkSemaphore> &signalingSemaphores = allSignallingSemaphores[cmdSubmitIdx];
        signalingSemaphores.resize(commands[cmdSubmitIdx].signalSemaphores.size());

        for (int32 i = 0; i < commands[cmdSubmitIdx].cmdBuffers.size(); ++i)
        {
            const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(commands[cmdSubmitIdx].cmdBuffers[i]);
            VulkanCommandPool &cmdPool = getPool(vCmdBuffer->fromQueue);
            cmdBuffers[i] = vCmdBuffer->cmdBuffer;
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
            waitOnSemaphores[i] = commands[cmdSubmitIdx].waitOn[i].waitOnSemaphore.reference<VulkanSemaphore>()->semaphore;
            waitingStages[i]
                = VkPipelineStageFlags(EngineToVulkanAPI::vulkanPipelineStageFlags(commands[cmdSubmitIdx].waitOn[i].stagesThatWaits));
        }
        for (int32 i = 0; i < commands[cmdSubmitIdx].signalSemaphores.size(); ++i)
        {
            signalingSemaphores[i] = commands[cmdSubmitIdx].signalSemaphores[i].reference<VulkanSemaphore>()->semaphore;
        }

        allSubmitInfo[cmdSubmitIdx].commandBufferCount = uint32(cmdBuffers.size());
        allSubmitInfo[cmdSubmitIdx].pCommandBuffers = cmdBuffers.data();
        allSubmitInfo[cmdSubmitIdx].signalSemaphoreCount = uint32(signalingSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pSignalSemaphores = signalingSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].waitSemaphoreCount = uint32(waitOnSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pWaitSemaphores = waitOnSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].pWaitDstStageMask = waitingStages.data();
    }

    if (!cmdsCompleteFence.isValid())
    {
        cmdsCompleteFence = graphicsHelper->createFence(graphicsInstance, TCHAR("AdvancedSubmitBatched"));
        cmdsCompleteFence->init();
    }
    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result
        = vDevice->vkQueueSubmit(vQueue, uint32(allSubmitInfo.size()), allSubmitInfo.data(), cmdsCompleteFence.reference<VulkanFence>()->fence);
    fatalAssertf(result == VK_SUCCESS, "Failed submitting command to queue %s(result: %d)", queueRes->getResourceName().getChar(), result);

    for (const CommandSubmitInfo &command : commands)
    {
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
            syncInfo.signalingSemaphore = command.signalSemaphores.empty() ? nullptr : command.signalSemaphores.front();
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

    std::vector<VkCommandBuffer> cmdBuffers(command.cmdBuffers.size());
    std::vector<VkSemaphore> waitOnSemaphores(command.waitOn.size());
    std::vector<VkPipelineStageFlags> waitingStages(command.waitOn.size());
    std::vector<VkSemaphore> signallingSemaphores(command.signalSemaphores.size());

    for (int32 i = 0; i < command.cmdBuffers.size(); ++i)
    {
        const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(command.cmdBuffers[i]);
        auto &cmdPool = getPool(vCmdBuffer->fromQueue);
        cmdBuffers[i] = vCmdBuffer->cmdBuffer;
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
        waitOnSemaphores[i] = command.waitOn[i].waitOnSemaphore.reference<VulkanSemaphore>()->semaphore;
        waitingStages[i] = VkPipelineStageFlags(EngineToVulkanAPI::vulkanPipelineStageFlags(command.waitOn[i].stagesThatWaits));
    }
    for (int32 i = 0; i < command.signalSemaphores.size(); ++i)
    {
        signallingSemaphores[i] = command.signalSemaphores[i].reference<VulkanSemaphore>()->semaphore;
    }

    SUBMIT_INFO(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferCount = uint32(cmdBuffers.size());
    cmdSubmitInfo.pCommandBuffers = cmdBuffers.data();
    cmdSubmitInfo.signalSemaphoreCount = uint32(signallingSemaphores.size());
    cmdSubmitInfo.pSignalSemaphores = signallingSemaphores.data();
    cmdSubmitInfo.waitSemaphoreCount = uint32(waitOnSemaphores.size());
    cmdSubmitInfo.pWaitSemaphores = waitOnSemaphores.data();
    cmdSubmitInfo.pWaitDstStageMask = waitingStages.data();

    if (!cmdsCompleteFence.isValid())
    {
        cmdsCompleteFence = graphicsHelper->createFence(graphicsInstance, TCHAR("AdvancedSubmitBatched"));
        cmdsCompleteFence->init();
    }
    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(
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
        syncInfo.signalingSemaphore = command.signalSemaphores.empty() ? nullptr : command.signalSemaphores.front();
        syncInfo.completeFence = cmdsCompleteFence;
        syncInfo.refCount = uint32(command.cmdBuffers.size());
    }
    else
    {
        cmdsSyncInfo.reset(index);
    }
}

void VulkanCmdBufferManager::submitCmds(
    EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2> &commands, VulkanResourcesTracker *resourceTracker
)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = IVulkanRHIModule::get()->getGraphicsHelper();
    QueueResourceBase *queueRes = nullptr;

    std::vector<std::vector<VkCommandBuffer>> allCmdBuffers(commands.size());
    std::vector<std::vector<VkSemaphore>> allWaitOnSemaphores(commands.size());
    std::vector<std::vector<VkPipelineStageFlags>> allWaitingStages(commands.size());
    std::vector<std::vector<VkSemaphore>> allSignalingSemaphores(commands.size());
    std::vector<VkSubmitInfo> allSubmitInfo(commands.size());

    // fill command buffer vector, all wait informations and make sure there is no error so far
    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        std::vector<VkCommandBuffer> &cmdBuffers = allCmdBuffers[cmdSubmitIdx];
        cmdBuffers.resize(commands[cmdSubmitIdx].cmdBuffers.size());
        std::vector<VkSemaphore> &waitOnSemaphores = allWaitOnSemaphores[cmdSubmitIdx];
        std::vector<VkPipelineStageFlags> &waitingStages = allWaitingStages[cmdSubmitIdx];

        for (int32 i = 0; i < commands[cmdSubmitIdx].cmdBuffers.size(); ++i)
        {
            const auto *vCmdBuffer = static_cast<const VulkanCommandBuffer *>(commands[cmdSubmitIdx].cmdBuffers[i]);
            if (vCmdBuffer->bIsTempBuffer)
            {
                LOG_ERROR(
                    "VulkanCommandBufferManager",
                    "Temporary buffers[%s] are required to use advanced submit "
                    "function",
                    vCmdBuffer->getResourceName().getChar()
                );
                return;
            }

            VulkanCommandPool &cmdPool = getPool(vCmdBuffer->fromQueue);
            cmdBuffers[i] = vCmdBuffer->cmdBuffer;
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
                            "VulkanCommandBufferManager",
                            "Waiting on cmd buffer[%s] is invalid or not "
                            "submitted",
                            waitOn.cmdBuffer->getResourceName().getChar()
                        );
                        return;
                    }

                    const VulkanCmdSubmitSyncInfo &syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
                    waitOnSemaphores.emplace_back(syncInfo.signalingSemaphore.reference<VulkanSemaphore>()->semaphore);
                    waitingStages.emplace_back(waitOn.usedDstStages);
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
            waitOnSemaphores.emplace_back(syncInfo.signalingSemaphore.reference<VulkanSemaphore>()->semaphore);
            waitingStages.emplace_back(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        }

        allSubmitInfo[cmdSubmitIdx].commandBufferCount = uint32(cmdBuffers.size());
        allSubmitInfo[cmdSubmitIdx].pCommandBuffers = cmdBuffers.data();
        allSubmitInfo[cmdSubmitIdx].waitSemaphoreCount = uint32(waitOnSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pWaitSemaphores = waitOnSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].pWaitDstStageMask = waitingStages.data();
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

        syncInfo.signalingSemaphore
            = graphicsHelper->createSemaphore(graphicsInstance, (TCHAR("SubmitBatched_") + String::toString(cmdSubmitIdx)).c_str());
        syncInfo.signalingSemaphore->init();

        std::vector<VkSemaphore> &signalingSemaphores = allSignalingSemaphores[cmdSubmitIdx];
        signalingSemaphores.emplace_back(syncInfo.signalingSemaphore.reference<VulkanSemaphore>()->semaphore);

        allSubmitInfo[cmdSubmitIdx].signalSemaphoreCount = uint32(signalingSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pSignalSemaphores = signalingSemaphores.data();
    }

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(
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

    std::vector<VkCommandBuffer> cmdBuffers(command.cmdBuffers.size());
    std::vector<VkSemaphore> waitOnSemaphores;
    std::vector<VkPipelineStageFlags> waitingStages;
    std::vector<VkSemaphore> signalingSemaphores;

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

        auto &cmdPool = getPool(vCmdBuffer->fromQueue);
        cmdBuffers[i] = vCmdBuffer->cmdBuffer;
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
                waitOnSemaphores.emplace_back(syncInfo.signalingSemaphore.reference<VulkanSemaphore>()->semaphore);
                waitingStages.emplace_back(waitOn.usedDstStages);
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
        waitOnSemaphores.emplace_back(syncInfo.signalingSemaphore.reference<VulkanSemaphore>()->semaphore);
        waitingStages.emplace_back(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
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

    syncInfo.signalingSemaphore = graphicsHelper->createSemaphore(graphicsInstance, TCHAR("SubmitSemaphore"));
    syncInfo.signalingSemaphore->init();
    signalingSemaphores.emplace_back(syncInfo.signalingSemaphore.reference<VulkanSemaphore>()->semaphore);

    SUBMIT_INFO(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferCount = uint32(cmdBuffers.size());
    cmdSubmitInfo.pCommandBuffers = cmdBuffers.data();
    cmdSubmitInfo.signalSemaphoreCount = uint32(signalingSemaphores.size());
    cmdSubmitInfo.pSignalSemaphores = signalingSemaphores.data();
    cmdSubmitInfo.waitSemaphoreCount = uint32(waitOnSemaphores.size());
    cmdSubmitInfo.pWaitSemaphores = waitOnSemaphores.data();
    cmdSubmitInfo.pWaitDstStageMask = waitingStages.data();

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(
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

        auto newEnd = std::remove_if(
            resAccessorItr->second.lastReadsIn.begin(), resAccessorItr->second.lastReadsIn.end(),
            [cmdBuffer](const GraphicsResource *cmd)
            {
                return cmd == cmdBuffer;
            }
        );
        resAccessorItr->second.lastReadsIn.erase(newEnd, resAccessorItr->second.lastReadsIn.end());

        if (resAccessorItr->second.lastWrite == nullptr && resAccessorItr->second.lastReadsIn.empty())
        {
            resAccessorItr = resourcesAccessors.erase(resAccessorItr);
        }
        else
        {
            ++resAccessorItr;
        }
    }
}

void VulkanResourcesTracker::clearResource(const MemoryResourceRef &resource) { resourcesAccessors.erase(resource); }

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
        if (memResources.find(itr->first.reference()) == memResources.end())
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
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readOnlyBuffers(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    if (!accessors.lastWrite)
    {
        accessors.lastReadsIn.emplace_back(cmdBuffer);
        accessors.allReadStages |= resource.second;
        accessors.lastReadStages = resource.second;
        return outBarrierInfo;
    }

    if (accessors.lastWrite == cmdBuffer)
    {
        // If this is the first barrier within this command for this resource
        if (accessors.lastReadsIn.empty())
        {
            ResourceBarrierInfo barrier;
            barrier.accessors.lastWrite = accessors.lastWrite;
            barrier.accessors.lastWriteStage = accessors.lastWriteStage;
            barrier.resource = resource.first;

            outBarrierInfo = barrier;
        }
    }
    else
    {
        cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, resource.second });
    }
    accessors.lastReadsIn.emplace_back(cmdBuffer);
    accessors.allReadStages |= resource.second;
    accessors.lastReadStages = resource.second;
    return outBarrierInfo;
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readOnlyImages(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    if (!accessors.lastWrite)
    {
        accessors.lastReadsIn.emplace_back(cmdBuffer);
        accessors.allReadStages |= resource.second;
        accessors.lastReadStages = resource.second;
        return outBarrierInfo;
    }

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
        // If layout transition is not done on this cmd buffer, then wait on it as well
        if (accessors.lastReadsIn.front() != cmdBuffer)
        {
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastReadsIn.front(), resource.second });
        }
    }
    accessors.lastReadsIn.emplace_back(cmdBuffer);
    accessors.allReadStages |= resource.second;
    accessors.lastReadStages = resource.second;
    return outBarrierInfo;
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readOnlyTexels(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readFromWriteBuffers(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readFromWriteImages(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    return readOnlyImages(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readFromWriteTexels(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeReadOnlyBuffers(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    fatalAssertf(PlatformFunctions::getSetBitCount(resource.second) == 1, "Writing to buffer in several pipeline stages is incorrect");

    std::optional<ResourceBarrierInfo> outBarrierInfo;
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits(resource.second);
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    // If never read or write
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        return outBarrierInfo;
    }

    // If we are already reading in this cmd buffer then all other steps are already done so wait for
    // just read to finish
    if (std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) != accessors.lastReadsIn.cend())
    {
        // TODO(Jeslas): Check if cmd not waiting on other reads is an issue here?
        ResourceBarrierInfo barrier;
        barrier.accessors.lastReadsIn.emplace_back(cmdBuffer);
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
        for (const GraphicsResource *readInCmdBuffer : accessors.lastReadsIn)
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ readInCmdBuffer, resource.second });

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        accessors.lastReadsIn.clear();
        accessors.lastReadStages = 0;
        accessors.allReadStages = 0;
        // we do not have to wait for last write as reads already do that
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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeReadOnlyImages(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    fatalAssertf(PlatformFunctions::getSetBitCount(resource.second) == 1, "Writing to image in several pipeline stages is incorrect");

    std::optional<ResourceBarrierInfo> outBarrierInfo;
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits(resource.second);
    ResourceAccessors &accessors = resourcesAccessors[resource.first];
    // If never read or write
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        // Since image layout for Read/writes img depends on caller, use empty read write case to
        // handle it
        ResourceBarrierInfo barrier;

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;

        outBarrierInfo = barrier;
        return outBarrierInfo;
    }

    // If we are already reading in this cmd buffer then all other steps are already done so wait for
    // just read to finish
    if (std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) != accessors.lastReadsIn.cend())
    {
        // Since same command buffer we need to write after waiting for read
        ResourceBarrierInfo barrier;
        barrier.accessors.lastReadsIn.emplace_back(cmdBuffer);
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

    if (!accessors.lastReadsIn.empty()) // If not empty then there is other cmds that are reading so wait
                                        // for those cmds, and transfer layout
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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeReadOnlyTexels(
    const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource
)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo>
    VulkanResourcesTracker::writeBuffers(const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo>
    VulkanResourcesTracker::writeImages(const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource)
{
    return writeReadOnlyImages(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo>
    VulkanResourcesTracker::writeTexels(const GraphicsResource *cmdBuffer, const std::pair<MemoryResourceRef, VkPipelineStageFlags> &resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo>
    VulkanResourcesTracker::imageToGeneralLayout(const GraphicsResource *cmdBuffer, const ImageResourceRef resource)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;

    auto accessorsItr = resourcesAccessors.find(resource);
    if (accessorsItr != resourcesAccessors.end())
    {
        if (accessorsItr->second.lastWrite || !accessorsItr->second.lastReadsIn.empty())
        {
            outBarrierInfo = ResourceBarrierInfo();
            outBarrierInfo->accessors = accessorsItr->second;
            outBarrierInfo->resource = resource;
        }
        accessorsItr->second.allReadStages = accessorsItr->second.lastReadStages = 0;
        accessorsItr->second.lastReadsIn.clear();
        accessorsItr->second.lastWrite = nullptr;
    }

    return outBarrierInfo;
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo>
    VulkanResourcesTracker::colorAttachmentWrite(const GraphicsResource *cmdBuffer, const ImageResourceRef resource)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ResourceAccessors &accessors = resourcesAccessors[resource];

    // If never read or write, no need to do any transition unless we are loading in render pass
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;

        return outBarrierInfo;
    }

    // If not read in same cmd buffer then there is other cmds that are reading so wait for those cmds,
    // Transition is not necessary as load/clear either way layout will be compatible
    if (!accessors.lastReadsIn.empty()
        && std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) == accessors.lastReadsIn.cend())
    {
        for (const GraphicsResource *readInCmdBuffer : accessors.lastReadsIn)
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ readInCmdBuffer, VkPipelineStageFlags(stageFlag) });

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
        cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ accessors.lastWrite, VkPipelineStageFlags(stageFlag) });
    }
    accessors.lastWrite = cmdBuffer;
    accessors.lastWriteStage = stageFlag;
    return outBarrierInfo;
}
