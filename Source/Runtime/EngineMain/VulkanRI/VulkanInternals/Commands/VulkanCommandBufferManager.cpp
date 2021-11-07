#include "VulkanCommandBufferManager.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../../Core/Platform/PlatformFunctions.h"
#include "../Resources/VulkanQueueResource.h"
#include "../VulkanDevice.h"
#include "../Resources/VulkanSyncResource.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../Resources/VulkanMemoryResources.h"

#include <optional>
#include <unordered_set>

template<typename QueueResType, EQueuePriority::Enum Priority>
struct GetQueueOfPriority
{
    VkQueue operator()(QueueResType* queueRes)
    {
        return queueRes->getQueueOfPriority<Priority>();
    }
};

template<typename QueueResType>
using GetQueueOfPriorityLow = GetQueueOfPriority<QueueResType, EQueuePriority::Low>;
template<typename QueueResType>
using GetQueueOfPriorityMedium = GetQueueOfPriority<QueueResType, EQueuePriority::Medium>;
template<typename QueueResType>
using GetQueueOfPriorityHigh = GetQueueOfPriority<QueueResType, EQueuePriority::High>;
template<typename QueueResType>
using GetQueueOfPrioritySuperHigh = GetQueueOfPriority<QueueResType, EQueuePriority::SuperHigh>;

template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction>* getQueue(const VulkanDevice* device);

//////////////////////////////////////////////////////////////////////////
////  VulkanCommandBuffer
//////////////////////////////////////////////////////////////////////////

class VulkanCommandBuffer final : public GraphicsResource, public IVulkanResources
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
    void setResourceName(const String& name) override;
    /* IVulkanResources overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    /* Override ends */
};

#if EXPERIMENTAL

VkCommandBuffer VulkanGraphicsHelper::getRawCmdBuffer(class IGraphicsInstance* graphicsInstance, const GraphicsResource* cmdBuffer)
{
    if (cmdBuffer->getType()->isChildOf<VulkanCommandBuffer>())
    {
        return static_cast<const VulkanCommandBuffer*>(cmdBuffer)->cmdBuffer;
    }
    return nullptr;
}

#endif

DEFINE_VK_GRAPHICS_RESOURCE(VulkanCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER)

String VulkanCommandBuffer::getObjectName() const
{
    return getResourceName();
}

String VulkanCommandBuffer::getResourceName() const
{
    return bufferName;
}

void VulkanCommandBuffer::setResourceName(const String& name)
{
    bufferName = name;
}

uint64 VulkanCommandBuffer::getDispatchableHandle() const
{
    return uint64(cmdBuffer);
}

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
        Logger::error("VulkanCommandPool", "%s() : Command pool information is invalid", __func__);
        return;
    }
    release();
    BaseType::reinitResources();

    CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
    commandPoolCreateInfo.queueFamilyIndex = cmdPoolInfo.vulkanQueueIndex;

    commandPoolCreateInfo.flags = 0;
    if (cmdPoolInfo.vDevice->vkCreateCommandPool(cmdPoolInfo.logicalDevice, &commandPoolCreateInfo, nullptr, &oneTimeRecordPool) != VK_SUCCESS)
    {
        Logger::error("VulkanCommandPool", "%s() : Failed creating one time record command buffer pool", __func__);
        oneTimeRecordPool = nullptr;
    }
    else
    {
        cmdPoolInfo.vDevice->debugGraphics()->markObject((uint64)oneTimeRecordPool, getResourceName() + "_OneTimeRecordPool", getObjectType());
    }

    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if(cmdPoolInfo.vDevice->vkCreateCommandPool(cmdPoolInfo.logicalDevice, &commandPoolCreateInfo, nullptr, &tempCommandsPool) != VK_SUCCESS)
    {
        Logger::error("VulkanCommandPool", "%s() : Failed creating temporary one time use command buffer pool", __func__);
        tempCommandsPool = nullptr;
    }
    else
    {
        cmdPoolInfo.vDevice->debugGraphics()->markObject((uint64)tempCommandsPool, getResourceName() + "_TempCmdsPool", getObjectType());
    }

    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if(cmdPoolInfo.vDevice->vkCreateCommandPool(cmdPoolInfo.logicalDevice, &commandPoolCreateInfo, nullptr, &rerecordableCommandPool) != VK_SUCCESS)
    {
        Logger::error("VulkanCommandPool", "%s() : Failed creating rerecordable command buffer pool", __func__);
        rerecordableCommandPool = nullptr;
    }
    else
    {
        cmdPoolInfo.vDevice->debugGraphics()->markObject((uint64)rerecordableCommandPool, getResourceName() + "_RerecordableCmdPool", getObjectType());
    }
}

void VulkanCommandPool::release()
{
    if (oneTimeRecordPool)
    {
        cmdPoolInfo.vDevice->vkResetCommandPool(cmdPoolInfo.logicalDevice, oneTimeRecordPool, VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        cmdPoolInfo.vDevice->vkDestroyCommandPool(cmdPoolInfo.logicalDevice, oneTimeRecordPool, nullptr);
        oneTimeRecordPool = nullptr;
    }
    if (rerecordableCommandPool)
    {
        cmdPoolInfo.vDevice->vkResetCommandPool(cmdPoolInfo.logicalDevice, rerecordableCommandPool, VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        cmdPoolInfo.vDevice->vkDestroyCommandPool(cmdPoolInfo.logicalDevice, rerecordableCommandPool, nullptr);
        rerecordableCommandPool = nullptr;
    }
    if (tempCommandsPool)
    {
        cmdPoolInfo.vDevice->vkResetCommandPool(cmdPoolInfo.logicalDevice, tempCommandsPool, VkCommandPoolResetFlagBits::VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        cmdPoolInfo.vDevice->vkDestroyCommandPool(cmdPoolInfo.logicalDevice, tempCommandsPool, nullptr);
        tempCommandsPool = nullptr;
    }

    BaseType::release();
}

String VulkanCommandPool::getResourceName() const
{
    return poolName;
}

void VulkanCommandPool::setResourceName(const String& name)
{
    poolName = name;
}

String VulkanCommandPool::getObjectName() const
{
    return getResourceName();
}

VkCommandPool VulkanCommandPool::getCommandPool(class VulkanCommandBuffer const* cmdBuffer) const
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

VulkanCmdBufferManager::VulkanCmdBufferManager(class VulkanDevice* vulkanDevice)
    : vDevice(vulkanDevice)
{
    createPools();
}

VulkanCmdBufferManager::~VulkanCmdBufferManager()
{
    for (const std::pair<const String, VulkanCmdBufferState>& cmdBuffer : commandBuffers)
    {
        if (cmdBuffer.second.cmdSyncInfoIdx != -1)
        {
            Logger::warn("VulkanCmdBufferManager", "%s: Command buffer %s is not finished, trying to finish it"
                , __func__, cmdBuffer.second.cmdBuffer->getResourceName().getChar());
            cmdFinished(cmdBuffer.second.cmdBuffer->getResourceName(), nullptr);
        }
        cmdBuffer.second.cmdBuffer->release();
        delete cmdBuffer.second.cmdBuffer;
    }
    for (std::pair<const EQueueFunction, VulkanCommandPool>& poolPair : pools)
    {
        poolPair.second.release();
    }
    pools.clear();
}

const GraphicsResource* VulkanCmdBufferManager::beginTempCmdBuffer(const String& cmdName, EQueueFunction usingQueue)
{
    VulkanCommandPool& cmdPool = getPool(usingQueue);
    
    CMD_BUFFER_ALLOC_INFO(cmdBuffAllocInfo);
    cmdBuffAllocInfo.commandPool = cmdPool.tempCommandsPool;
    cmdBuffAllocInfo.commandBufferCount = 1;

    auto* cmdBuffer = new VulkanCommandBuffer();
    cmdBuffer->setResourceName(cmdName);
    cmdBuffer->bIsTempBuffer = true;
    cmdBuffer->fromQueue = cmdPool.cmdPoolInfo.queueType;
    cmdBuffer->usage = usingQueue;

    fatalAssert(vDevice->vkAllocateCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), &cmdBuffAllocInfo
        , &cmdBuffer->cmdBuffer) == VK_SUCCESS, "Allocating temporary command buffer failed");
    cmdBuffer->init();
    vDevice->debugGraphics()->markObject(cmdBuffer);

    CMD_BUFFER_BEGIN_INFO(cmdBuffBeginInfo);
    cmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(cmdBuffer->cmdBuffer, &cmdBuffBeginInfo);
    vDevice->debugGraphics()->beginCmdBufferMarker(cmdBuffer->cmdBuffer, cmdName);

    return cmdBuffer;
}

const GraphicsResource* VulkanCmdBufferManager::beginRecordOnceCmdBuffer(const String& cmdName, EQueueFunction usingQueue)
{
    VulkanCommandBuffer* cmdBuffer = nullptr;

    auto cmdBufferItr = commandBuffers.find(cmdName);
    if(cmdBufferItr == commandBuffers.end())
    {
        VulkanCommandPool& cmdPool = getPool(usingQueue);

        CMD_BUFFER_ALLOC_INFO(cmdBuffAllocInfo);
        cmdBuffAllocInfo.commandPool = cmdPool.oneTimeRecordPool;
        cmdBuffAllocInfo.commandBufferCount = 1;

        cmdBuffer = new VulkanCommandBuffer();
        cmdBuffer->setResourceName(cmdName);
        cmdBuffer->fromQueue = cmdPool.cmdPoolInfo.queueType;
        cmdBuffer->usage = usingQueue;

        fatalAssert(vDevice->vkAllocateCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), &cmdBuffAllocInfo
            , &cmdBuffer->cmdBuffer) == VK_SUCCESS, "Allocating record once command buffer failed");
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
            Logger::error("VulkanCommandBufferManager", "%s() : Trying to record a prerecorded command again is restricted Command = [%s]", __func__, cmdName.getChar());
            fatalAssert(false, "Cannot record prerecorded command again");
            return cmdBufferItr->second.cmdBuffer;
        case ECmdState::Recording:
            Logger::warn("VulkanCommandBufferManager", "%s() : Command %s is already being recorded", __func__, cmdName.getChar());
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

const GraphicsResource* VulkanCmdBufferManager::beginReuseCmdBuffer(const String& cmdName, EQueueFunction usingQueue)
{
    VulkanCommandBuffer* cmdBuffer = nullptr;

    auto cmdBufferItr = commandBuffers.find(cmdName);
    if (cmdBufferItr == commandBuffers.end())
    {
        VulkanCommandPool& cmdPool = getPool(usingQueue);

        CMD_BUFFER_ALLOC_INFO(cmdBuffAllocInfo);
        cmdBuffAllocInfo.commandPool = cmdPool.rerecordableCommandPool;
        cmdBuffAllocInfo.commandBufferCount = 1;

        cmdBuffer = new VulkanCommandBuffer();
        cmdBuffer->setResourceName(cmdName);
        cmdBuffer->bIsResetable = true;
        cmdBuffer->fromQueue = cmdPool.cmdPoolInfo.queueType;
        cmdBuffer->usage = usingQueue;

        fatalAssert(vDevice->vkAllocateCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), &cmdBuffAllocInfo
            , &cmdBuffer->cmdBuffer) == VK_SUCCESS, "Allocating reusable command buffer failed");
        cmdBuffer->init();
        vDevice->debugGraphics()->markObject(cmdBuffer);

        commandBuffers[cmdName] = { cmdBuffer, ECmdState::Recording };
    }
    else
    {
        switch (cmdBufferItr->second.cmdState)
        {
        case ECmdState::Submitted:
            Logger::error("VulkanCommandBufferManager", "%s() : Trying to record a submitted command [%s] is restricted before it is finished", __func__, cmdName.getChar());
            fatalAssert(false, "Cannot record command while it is still executing");
            return cmdBufferItr->second.cmdBuffer;
        case ECmdState::Recording:
            Logger::warn("VulkanCommandBufferManager", "%s() : Command [%s] is already being recorded", __func__, cmdName.getChar());
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

void VulkanCmdBufferManager::startRenderPass(const GraphicsResource* cmdBuffer)
{
    const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
        if (cmdBufferItr != commandBuffers.end())
        {
            fatalAssert(cmdBufferItr->second.cmdState == ECmdState::Recording, "%s: %s cmd buffer is not recording to start render pass", __func__, cmdBufferItr->first.getChar());
            cmdBufferItr->second.cmdState = ECmdState::RenderPass;
        }
    }
}

bool VulkanCmdBufferManager::isInRenderPass(const GraphicsResource* cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    if (cmdBufferItr != commandBuffers.end())
    {
        return cmdBufferItr->second.cmdState == ECmdState::RenderPass;
    }

    return false;
}

void VulkanCmdBufferManager::endRenderPass(const GraphicsResource* cmdBuffer)
{
    const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
        if (cmdBufferItr != commandBuffers.end() && cmdBufferItr->second.cmdState == ECmdState::RenderPass)
        {
            cmdBufferItr->second.cmdState = ECmdState::Recording;
        }
    }
}

void VulkanCmdBufferManager::endCmdBuffer(const GraphicsResource* cmdBuffer)
{
    const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(cmdBuffer);
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

void VulkanCmdBufferManager::cmdFinished(const GraphicsResource* cmdBuffer, VulkanResourcesTracker* resourceTracker)
{
    const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(cmdBuffer);

    cmdFinished(cmdBuffer->getResourceName(), resourceTracker);
}

void VulkanCmdBufferManager::cmdFinished(const String& cmdName, VulkanResourcesTracker* resourceTracker)
{
    auto cmdBufferItr = commandBuffers.find(cmdName);
    // If submitted then only it can be finished in queue
    if (cmdBufferItr != commandBuffers.end() && cmdBufferItr->second.cmdState == ECmdState::Submitted)
    {
        VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
        syncInfo.refCount--;
        if (!syncInfo.bIsAdvancedSubmit && syncInfo.completeFence)
        {
            if (!syncInfo.completeFence->isSignaled())
            {
                syncInfo.completeFence->waitForSignal();
            }

            if (syncInfo.refCount == 0)
            {
                syncInfo.completeFence->resetSignal();
                syncInfo.completeFence->release();
            }
        }
        if (syncInfo.refCount == 0)
        {
            if (!syncInfo.bIsAdvancedSubmit)
            {
                syncInfo.signalingSemaphore->release();
            }
            cmdsSyncInfo.reset(cmdBufferItr->second.cmdSyncInfoIdx);
        }
        if (resourceTracker)
        {
            resourceTracker->clearFinishedCmd(cmdBufferItr->second.cmdBuffer);
        }
        cmdBufferItr->second.cmdSyncInfoIdx = -1;
        cmdBufferItr->second.cmdState = ECmdState::Recorded;
    }
}

void VulkanCmdBufferManager::finishAllSubmited(VulkanResourcesTracker* resourceTracker)
{
    for (const std::pair<const String, VulkanCmdBufferState>& cmd : commandBuffers)
    {
        if (cmd.second.cmdState == ECmdState::Submitted)
        {
            VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[cmd.second.cmdSyncInfoIdx];
            // If advanced submit then finishing won't wait so wait here
            if (syncInfo.bIsAdvancedSubmit && !syncInfo.completeFence->isSignaled())
            {
                syncInfo.completeFence->waitForSignal();
            }
            cmdFinished(cmd.second.cmdBuffer->getResourceName(), resourceTracker);
        }
    }
}

void VulkanCmdBufferManager::freeCmdBuffer(const GraphicsResource* cmdBuffer)
{
    const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(cmdBuffer);
    VulkanCommandPool& cmdPool = getPool(vCmdBuffer->fromQueue);

    vDevice->vkFreeCommandBuffers(VulkanGraphicsHelper::getDevice(vDevice), cmdPool.getCommandPool(vCmdBuffer), 1, &vCmdBuffer->cmdBuffer);
    if (!vCmdBuffer->bIsTempBuffer)
    {
        commandBuffers.erase(cmdBuffer->getResourceName());
    }

    const_cast<GraphicsResource*>(cmdBuffer)->release();
    delete cmdBuffer;
}

VkCommandBuffer VulkanCmdBufferManager::getRawBuffer(const GraphicsResource* cmdBuffer) const
{
    return cmdBuffer->getType()->isChildOf<VulkanCommandBuffer>() ? static_cast<const VulkanCommandBuffer*>(cmdBuffer)->cmdBuffer : nullptr;
}

const GraphicsResource* VulkanCmdBufferManager::getCmdBuffer(const String& cmdName) const
{
    auto cmdBufferItr = commandBuffers.find(cmdName);
    if (cmdBufferItr != commandBuffers.end())
    {
        return cmdBufferItr->second.cmdBuffer;
    }
    return nullptr;
}

uint32 VulkanCmdBufferManager::getQueueFamilyIdx(EQueueFunction queue) const
{
    return pools.at(queue).cmdPoolInfo.vulkanQueueIndex;
}

uint32 VulkanCmdBufferManager::getQueueFamilyIdx(const GraphicsResource* cmdBuffer) const
{
    return getQueueFamilyIdx(static_cast<const VulkanCommandBuffer*>(cmdBuffer)->fromQueue);
}

ECmdState VulkanCmdBufferManager::getState(const GraphicsResource* cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    if (cmdBufferItr != commandBuffers.cend())
    {
        return cmdBufferItr->second.cmdState;
    }
    Logger::debug("VulkanCmdBufferManager", "%s() : Not available command buffer[%s] queried for state", __func__, cmdBuffer->getResourceName().getChar());
    return ECmdState::Idle;
}

SharedPtr<GraphicsSemaphore> VulkanCmdBufferManager::cmdSignalSemaphore(const GraphicsResource* cmdBuffer) const
{
    auto cmdBufferItr = commandBuffers.find(cmdBuffer->getResourceName());
    if (cmdBufferItr != commandBuffers.cend() && cmdBufferItr->second.cmdSyncInfoIdx >= 0)
    {
        return cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx].signalingSemaphore;
    }
    return nullptr;
}

bool VulkanCmdBufferManager::isComputeCmdBuffer(const GraphicsResource* cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer*>(cmdBuffer)->usage == EQueueFunction::Compute;
}

bool VulkanCmdBufferManager::isGraphicsCmdBuffer(const GraphicsResource* cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer*>(cmdBuffer)->usage == EQueueFunction::Graphics;
}

bool VulkanCmdBufferManager::isTransferCmdBuffer(const GraphicsResource* cmdBuffer) const
{
    return static_cast<const VulkanCommandBuffer*>(cmdBuffer)->usage == EQueueFunction::Transfer;
}

void VulkanCmdBufferManager::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo>& commands, const SharedPtr<GraphicsFence>& cmdsCompleteFence)
{
    QueueResourceBase* queueRes = nullptr;

    std::vector<std::vector<VkCommandBuffer>> allCmdBuffers(commands.size());
    std::vector<std::vector<VkSemaphore>> allWaitOnSemaphores(commands.size());
    std::vector<std::vector<VkPipelineStageFlags>> allWaitingStages(commands.size());
    std::vector<std::vector<VkSemaphore>> allSignallingSemaphores(commands.size());
    std::vector<VkSubmitInfo> allSubmitInfo(commands.size());

    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        std::vector<VkCommandBuffer>& cmdBuffers = allCmdBuffers[cmdSubmitIdx];
        cmdBuffers.resize(commands[cmdSubmitIdx].cmdBuffers.size());
        std::vector<VkSemaphore>& waitOnSemaphores = allWaitOnSemaphores[cmdSubmitIdx];
        waitOnSemaphores.resize(commands[cmdSubmitIdx].waitOn.size());
        std::vector<VkPipelineStageFlags>& waitingStages = allWaitingStages[cmdSubmitIdx];
        waitingStages.resize(waitOnSemaphores.size());
        std::vector<VkSemaphore>& signalingSemaphores = allSignallingSemaphores[cmdSubmitIdx];
        signalingSemaphores.resize(commands[cmdSubmitIdx].signalSemaphores.size());

        for (int32 i = 0; i < commands[cmdSubmitIdx].cmdBuffers.size(); ++i)
        {
            const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(commands[cmdSubmitIdx].cmdBuffers[i]);
            VulkanCommandPool& cmdPool = getPool(vCmdBuffer->fromQueue);
            cmdBuffers[i] = vCmdBuffer->cmdBuffer;
            if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
            {
                Logger::error("VulkanCommandBufferManager", "%s() : Buffers from different queues cannot be submitted together", __func__);
                return;
            }
            queueRes = cmdPool.cmdPoolInfo.queueResource;
        }
        if (queueRes == nullptr)
        {
            Logger::error("VulkanCommandBufferManager", "%s() : Cannot submit as there is no queue found for command buffers", __func__);
            return;
        }

        for (int32 i = 0; i < commands[cmdSubmitIdx].waitOn.size(); ++i)
        {
            waitOnSemaphores[i] = static_cast<VulkanSemaphore*>(commands[cmdSubmitIdx].waitOn[i].waitOnSemaphore.get())->semaphore;
            waitingStages[i] = commands[cmdSubmitIdx].waitOn[i].stagesThatWaits;
        }
        for (int32 i = 0; i < commands[cmdSubmitIdx].signalSemaphores.size(); ++i)
        {
            signalingSemaphores[i] = static_cast<VulkanSemaphore*>(commands[cmdSubmitIdx].signalSemaphores[i].get())->semaphore;
        }

        allSubmitInfo[cmdSubmitIdx].commandBufferCount = uint32(cmdBuffers.size());
        allSubmitInfo[cmdSubmitIdx].pCommandBuffers = cmdBuffers.data();
        allSubmitInfo[cmdSubmitIdx].signalSemaphoreCount = uint32(signalingSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pSignalSemaphores = signalingSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].waitSemaphoreCount = uint32(waitOnSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pWaitSemaphores = waitOnSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].pWaitDstStageMask = waitingStages.data();
    }

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(vQueue, uint32(allSubmitInfo.size()), allSubmitInfo.data(), (cmdsCompleteFence
        ? static_cast<VulkanFence*>(cmdsCompleteFence.get())->fence : nullptr));
    fatalAssert(result == VK_SUCCESS
        , "%s(): Failed submitting command to queue %s(result: %d)", __func__, queueRes->getResourceName().getChar(), result);

    for (const CommandSubmitInfo& command : commands)
    {
        bool bAnyNonTemp = false;
        int32 index = int32(cmdsSyncInfo.get());
        VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[index];
        for (const GraphicsResource* cmdBuffer : command.cmdBuffers)
        {
            if (!static_cast<const VulkanCommandBuffer*>(cmdBuffer)->bIsTempBuffer)
            {
                bAnyNonTemp = true;
                commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
                commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
            }
        }
        if (bAnyNonTemp)
        {
            syncInfo.signalingSemaphore = command.signalSemaphores.front();
            syncInfo.completeFence = cmdsCompleteFence;
            syncInfo.bIsAdvancedSubmit = true;
            syncInfo.refCount = uint32(command.cmdBuffers.size());
        }
        else
        {
            cmdsSyncInfo.reset(index);
        }
    }
}

void VulkanCmdBufferManager::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& command, const SharedPtr<GraphicsFence>& cmdsCompleteFence)
{
    QueueResourceBase* queueRes = nullptr;

    std::vector<VkCommandBuffer> cmdBuffers(command.cmdBuffers.size());
    std::vector<VkSemaphore> waitOnSemaphores(command.waitOn.size());
    std::vector<VkPipelineStageFlags> waitingStages(command.waitOn.size());
    std::vector<VkSemaphore> signallingSemaphores(command.signalSemaphores.size());

    for (int32 i = 0; i < command.cmdBuffers.size(); ++i)
    {
        const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(command.cmdBuffers[i]);
        auto& cmdPool = getPool(vCmdBuffer->fromQueue);
        cmdBuffers[i] = vCmdBuffer->cmdBuffer;
        if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
        {
            Logger::error("VulkanCommandBufferManager", "%s() : Buffers from different queues cannot be submitted together", __func__);
            return;
        }
        queueRes = cmdPool.cmdPoolInfo.queueResource;
    }
    if (queueRes == nullptr)
    {
        Logger::error("VulkanCommandBufferManager", "%s() : Cannot submit as there is no queue found for command buffers", __func__);
        return;
    }

    for (int32 i = 0; i < command.waitOn.size(); ++i)
    {
        waitOnSemaphores[i] = static_cast<VulkanSemaphore*>(command.waitOn[i].waitOnSemaphore.get())->semaphore;
        waitingStages[i] = command.waitOn[i].stagesThatWaits;
    }
    for (int32 i = 0; i < command.signalSemaphores.size(); ++i)
    {
        signallingSemaphores[i] = static_cast<VulkanSemaphore*>(command.signalSemaphores[i].get())->semaphore;
    }

    SUBMIT_INFO(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferCount = uint32(cmdBuffers.size());
    cmdSubmitInfo.pCommandBuffers = cmdBuffers.data();
    cmdSubmitInfo.signalSemaphoreCount = uint32(signallingSemaphores.size());
    cmdSubmitInfo.pSignalSemaphores = signallingSemaphores.data();
    cmdSubmitInfo.waitSemaphoreCount = uint32(waitOnSemaphores.size());
    cmdSubmitInfo.pWaitSemaphores = waitOnSemaphores.data();
    cmdSubmitInfo.pWaitDstStageMask = waitingStages.data();


    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(vQueue, 1, &cmdSubmitInfo, (cmdsCompleteFence
        ? static_cast<VulkanFence*>(cmdsCompleteFence.get())->fence : nullptr));
    fatalAssert(result == VK_SUCCESS
        , "%s(): Failed submitting command to queue %s(result: %d)", __func__, queueRes->getResourceName().getChar(), result);


    bool bAnyNonTemp = false;
    int32 index = int32(cmdsSyncInfo.get());
    VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[index];
    for (const GraphicsResource* cmdBuffer : command.cmdBuffers)
    {
        if (!static_cast<const VulkanCommandBuffer*>(cmdBuffer)->bIsTempBuffer)
        {
            bAnyNonTemp = true;
            commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
            commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
        }
    }
    if (bAnyNonTemp)
    {
        syncInfo.signalingSemaphore = command.signalSemaphores.front();
        syncInfo.completeFence = cmdsCompleteFence;
        syncInfo.bIsAdvancedSubmit = true;
        syncInfo.refCount = uint32(command.cmdBuffers.size());
    }
    else
    {
        cmdsSyncInfo.reset(index);
    }
}

void VulkanCmdBufferManager::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands, VulkanResourcesTracker* resourceTracker)
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
    QueueResourceBase* queueRes = nullptr;

    std::vector<std::vector<VkCommandBuffer>> allCmdBuffers(commands.size());
    std::vector<std::vector<VkSemaphore>> allWaitOnSemaphores(commands.size());
    std::vector<std::vector<VkPipelineStageFlags>> allWaitingStages(commands.size());
    std::vector<std::vector<VkSemaphore>> allSignalingSemaphores(commands.size());
    std::vector<VkSubmitInfo> allSubmitInfo(commands.size());

    // fill command buffer vector, all wait informations and make sure there is no error so far
    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        std::vector<VkCommandBuffer>& cmdBuffers = allCmdBuffers[cmdSubmitIdx];
        cmdBuffers.resize(commands[cmdSubmitIdx].cmdBuffers.size());
        std::vector<VkSemaphore>& waitOnSemaphores = allWaitOnSemaphores[cmdSubmitIdx];
        std::vector<VkPipelineStageFlags>& waitingStages = allWaitingStages[cmdSubmitIdx];

        for (int32 i = 0; i < commands[cmdSubmitIdx].cmdBuffers.size(); ++i)
        {
            const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(commands[cmdSubmitIdx].cmdBuffers[i]);
            if (vCmdBuffer->bIsTempBuffer)
            {
                Logger::error("VulkanCommandBufferManager", "%s() : Temporary buffers[%s] are required to use advanced submit function", __func__
                    , vCmdBuffer->getResourceName().getChar());
                return;
            }

            VulkanCommandPool& cmdPool = getPool(vCmdBuffer->fromQueue);
            cmdBuffers[i] = vCmdBuffer->cmdBuffer;
            if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
            {
                Logger::error("VulkanCommandBufferManager", "%s() : Buffers from different queues cannot be submitted together", __func__);
                return;
            }
            queueRes = cmdPool.cmdPoolInfo.queueResource;

            // Resource tracked waits
            const std::vector<VulkanResourcesTracker::CommandResUsageInfo>* resWaits = resourceTracker->getCmdBufferDeps(vCmdBuffer);
            if(resWaits)
            {
                for (const VulkanResourcesTracker::CommandResUsageInfo& waitOn : *resWaits)
                {
                    auto cmdBufferItr = commandBuffers.find(waitOn.cmdBuffer->getResourceName());
                    if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
                    {
                        Logger::error("VulkanCommandBufferManager", "%s() : Waiting on cmd buffer[%s] is invalid or not submitted", __func__, waitOn.cmdBuffer->getResourceName().getChar());
                        return;
                    }

                    const VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
                    waitOnSemaphores.emplace_back(static_cast<VulkanSemaphore*>(syncInfo.signalingSemaphore.get())->semaphore);
                    waitingStages.emplace_back(waitOn.usageStages);
                }
            }
        }
        if (queueRes == nullptr)
        {
            Logger::error("VulkanCommandBufferManager", "%s() : Cannot submit as there is no queue found for command buffers", __func__);
            return;
        }

        // Manual waits
        for (const GraphicsResource* waitOn : commands[cmdSubmitIdx].waitOnCmdBuffers)
        {
            auto cmdBufferItr = commandBuffers.find(waitOn->getResourceName());
            if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
            {
                Logger::error("VulkanCommandBufferManager", "%s() : Waiting on cmd buffer[%s] is invalid or not submitted", __func__, waitOn->getResourceName().getChar());
                return;
            }

            const VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
            waitOnSemaphores.emplace_back(static_cast<VulkanSemaphore*>(syncInfo.signalingSemaphore.get())->semaphore);
            waitingStages.emplace_back(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        }

        allSubmitInfo[cmdSubmitIdx].commandBufferCount = uint32(cmdBuffers.size());
        allSubmitInfo[cmdSubmitIdx].pCommandBuffers = cmdBuffers.data();
        allSubmitInfo[cmdSubmitIdx].waitSemaphoreCount = uint32(waitOnSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pWaitSemaphores = waitOnSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].pWaitDstStageMask = waitingStages.data();
    }

    SharedPtr<GraphicsFence> cmdsCompleteFence = GraphicsHelper::createFence(graphicsInstance, "SubmitBatched");

    // Fill all signaling semaphores, Set cmd states
    for (int32 cmdSubmitIdx = 0; cmdSubmitIdx < commands.size(); ++cmdSubmitIdx)
    {
        int32 index = int32(cmdsSyncInfo.get());
        VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[index];
        syncInfo.bIsAdvancedSubmit = false;
        syncInfo.completeFence = cmdsCompleteFence;
        syncInfo.refCount = uint32(commands[cmdSubmitIdx].cmdBuffers.size());

        for (const GraphicsResource* cmdBuffer : commands[cmdSubmitIdx].cmdBuffers)
        {
            commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
            commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;

            // Remove dependencies if re-record able cmd buffer
            if (static_cast<const VulkanCommandBuffer*>(cmdBuffer)->bIsResetable)
            {
                resourceTracker->clearCmdBufferDeps(cmdBuffer);
            }
        }

        syncInfo.signalingSemaphore = GraphicsHelper::createSemaphore(graphicsInstance, ("SubmitBatched_" + std::to_string(cmdSubmitIdx)).c_str());
        std::vector<VkSemaphore>& signalingSemaphores = allSignalingSemaphores[cmdSubmitIdx];
        signalingSemaphores.emplace_back(static_cast<VulkanSemaphore*>(syncInfo.signalingSemaphore.get())->semaphore);

        allSubmitInfo[cmdSubmitIdx].signalSemaphoreCount = uint32(signalingSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pSignalSemaphores = signalingSemaphores.data();
    }

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(vQueue, uint32(allSubmitInfo.size()), allSubmitInfo.data(), (cmdsCompleteFence
        ? static_cast<VulkanFence*>(cmdsCompleteFence.get())->fence : nullptr));
    fatalAssert(result == VK_SUCCESS
        , "%s(): Failed submitting command to queue %s(result: %d)", __func__, queueRes->getResourceName().getChar(), result);
}

void VulkanCmdBufferManager::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command, VulkanResourcesTracker* resourceTracker)
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
    QueueResourceBase* queueRes = nullptr;

    std::vector<VkCommandBuffer> cmdBuffers(command.cmdBuffers.size());
    std::vector<VkSemaphore> waitOnSemaphores;
    std::vector<VkPipelineStageFlags> waitingStages;
    std::vector<VkSemaphore> signalingSemaphores;

    for (int32 i = 0; i < command.cmdBuffers.size(); ++i)
    {
        const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(command.cmdBuffers[i]);
        if (vCmdBuffer->bIsTempBuffer)
        {
            Logger::error("VulkanCommandBufferManager", "%s() : Temporary buffers[%s] are required to use advanced submit function", __func__
                , vCmdBuffer->getResourceName().getChar());
            return;
        }

        auto& cmdPool = getPool(vCmdBuffer->fromQueue);
        cmdBuffers[i] = vCmdBuffer->cmdBuffer;
        if (queueRes != nullptr && queueRes != cmdPool.cmdPoolInfo.queueResource)
        {
            Logger::error("VulkanCommandBufferManager", "%s() : Buffers from different queues cannot be submitted together", __func__);
            return;
        }
        queueRes = cmdPool.cmdPoolInfo.queueResource;

        // Resource tracked waits
        const std::vector<VulkanResourcesTracker::CommandResUsageInfo>* resWaits = resourceTracker->getCmdBufferDeps(vCmdBuffer);
        if (resWaits)
        {
            for (const VulkanResourcesTracker::CommandResUsageInfo& waitOn : *resWaits)
            {
                auto cmdBufferItr = commandBuffers.find(waitOn.cmdBuffer->getResourceName());
                if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
                {
                    Logger::error("VulkanCommandBufferManager", "%s() : Waiting on cmd buffer[%s] is invalid or not submitted", __func__, waitOn.cmdBuffer->getResourceName().getChar());
                    return;
                }

                const VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
                waitOnSemaphores.emplace_back(static_cast<VulkanSemaphore*>(syncInfo.signalingSemaphore.get())->semaphore);
                waitingStages.emplace_back(waitOn.usageStages);
            }
        }
    }
    if (queueRes == nullptr)
    {
        Logger::error("VulkanCommandBufferManager", "%s() : Cannot submit as there is no queue found for command buffers", __func__);
        return;
    }

    for (const GraphicsResource* waitOn : command.waitOnCmdBuffers)
    {
        auto cmdBufferItr = commandBuffers.find(waitOn->getResourceName());
        if (cmdBufferItr == commandBuffers.end() || cmdBufferItr->second.cmdState != ECmdState::Submitted)
        {
            Logger::error("VulkanCommandBufferManager", "%s() : Waiting on cmd buffer[%s] is invalid or not submitted", __func__, waitOn->getResourceName().getChar());
            return;
        }

        const VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[cmdBufferItr->second.cmdSyncInfoIdx];
        waitOnSemaphores.emplace_back(static_cast<VulkanSemaphore*>(syncInfo.signalingSemaphore.get())->semaphore);
        waitingStages.emplace_back(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    }

    SharedPtr<GraphicsFence> cmdsCompleteFence = GraphicsHelper::createFence(graphicsInstance, "SubmitBatched");

    int32 index = int32(cmdsSyncInfo.get());
    VulkanCmdSubmitSyncInfo& syncInfo = cmdsSyncInfo[index];
    syncInfo.bIsAdvancedSubmit = false;
    syncInfo.completeFence = cmdsCompleteFence;
    syncInfo.refCount = uint32(command.cmdBuffers.size());

    for (const GraphicsResource* cmdBuffer : command.cmdBuffers)
    {
        commandBuffers[cmdBuffer->getResourceName()].cmdSyncInfoIdx = index;
        commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;

        // Remove dependencies if re-record able cmd buffer
        if (static_cast<const VulkanCommandBuffer*>(cmdBuffer)->bIsResetable)
        {
            resourceTracker->clearCmdBufferDeps(cmdBuffer);
        }
    }

    syncInfo.signalingSemaphore = GraphicsHelper::createSemaphore(graphicsInstance, "SubmitSemaphore");
    signalingSemaphores.emplace_back(static_cast<VulkanSemaphore*>(syncInfo.signalingSemaphore.get())->semaphore);

    SUBMIT_INFO(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferCount = uint32(cmdBuffers.size());
    cmdSubmitInfo.pCommandBuffers = cmdBuffers.data();
    cmdSubmitInfo.signalSemaphoreCount = uint32(signalingSemaphores.size());
    cmdSubmitInfo.pSignalSemaphores = signalingSemaphores.data();
    cmdSubmitInfo.waitSemaphoreCount = uint32(waitOnSemaphores.size());
    cmdSubmitInfo.pWaitSemaphores = waitOnSemaphores.data();
    cmdSubmitInfo.pWaitDstStageMask = waitingStages.data();

    VkQueue vQueue = getVkQueue(priority, queueRes);
    VkResult result = vDevice->vkQueueSubmit(vQueue, 1, &cmdSubmitInfo, (cmdsCompleteFence
        ? static_cast<VulkanFence*>(cmdsCompleteFence.get())->fence : nullptr));
    fatalAssert(result == VK_SUCCESS
        , "%s(): Failed submitting command to queue %s(result: %d)", __func__, queueRes->getResourceName().getChar(), result);
}

void VulkanCmdBufferManager::createPools()
{
    VkDevice logicalDevice = VulkanGraphicsHelper::getDevice(vDevice);
    if (vDevice->getComputeQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Compute>* queue = static_cast<VulkanQueueResource<EQueueFunction::Compute>*>(vDevice->getComputeQueue());
        VulkanCommandPool& pool = pools[EQueueFunction::Compute];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Compute };
        pool.init();
    }

    if (vDevice->getGraphicsQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Graphics>* queue = static_cast<VulkanQueueResource<EQueueFunction::Graphics>*>(vDevice->getGraphicsQueue());
        VulkanCommandPool& pool = pools[EQueueFunction::Graphics];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Graphics };
        pool.init();
    }

    if (vDevice->getTransferQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Transfer>* queue = static_cast<VulkanQueueResource<EQueueFunction::Transfer>*>(vDevice->getTransferQueue());
        VulkanCommandPool& pool = pools[EQueueFunction::Transfer];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Transfer };
        pool.init();
    }

    if (vDevice->getGenericQueue() != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Generic>* queue = static_cast<VulkanQueueResource<EQueueFunction::Generic>*>(vDevice->getGenericQueue());
        VulkanCommandPool& pool = pools[EQueueFunction::Generic];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Generic };
        pool.init();
        genericPool = &pool;
    }

    if (getQueue<EQueueFunction::Present>(vDevice) != nullptr)
    {
        VulkanQueueResource<EQueueFunction::Present>* queue = getQueue<EQueueFunction::Present>(vDevice);
        VulkanCommandPool& pool = pools[EQueueFunction::Present];
        pool.setResourceName(queue->getSupportedQueueName());
        pool.cmdPoolInfo = { vDevice, logicalDevice, queue->queueFamilyIndex(), queue, EQueueFunction::Present };
        pool.init();
    }

}

VulkanCommandPool& VulkanCmdBufferManager::getPool(EQueueFunction forQueue)
{
    auto cmdPoolItr = pools.find(forQueue);
    if (cmdPoolItr == pools.end())
    {
        fatalAssert(genericPool, "Generic pool must be available");
        return *genericPool;
    }
    else
    {
        return cmdPoolItr->second;
    }
}

VkQueue VulkanCmdBufferManager::getVkQueue(EQueuePriority::Enum priority, QueueResourceBase* queueRes)
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

const std::vector<VulkanResourcesTracker::CommandResUsageInfo>* VulkanResourcesTracker::getCmdBufferDeps(const GraphicsResource* cmdBuffer) const
{
    CmdWaitInfoMap::const_iterator itr = cmdWaitInfo.find(cmdBuffer);
    if (itr != cmdWaitInfo.cend())
    {
        return &itr->second;
    }
    return nullptr;
}

void VulkanResourcesTracker::clearCmdBufferDeps(const GraphicsResource* cmdBuffer)
{
    cmdWaitInfo.erase(cmdBuffer);
}

void VulkanResourcesTracker::clearFinishedCmd(const GraphicsResource* cmdBuffer)
{
    cmdWaitInfo.erase(cmdBuffer);

    for (auto& resAccessor : resourcesAccessors)
    {
        if (resAccessor.second.lastWrite == cmdBuffer)
        {
            resAccessor.second.lastWrite = nullptr;
        }

        auto newEnd = std::remove_if(resAccessor.second.lastReadsIn.begin(), resAccessor.second.lastReadsIn.end()
            ,[cmdBuffer](const GraphicsResource* cmd)
            {
                return cmd == cmdBuffer;
            }
        );
        resAccessor.second.lastReadsIn.erase(newEnd, resAccessor.second.lastReadsIn.end());
    }


    for (auto& attachment : renderpassAttachments)
    {
        if (attachment.second.lastWrite == cmdBuffer)
        {
            attachment.second.lastWrite = nullptr;
        }

        auto newEnd = std::remove_if(attachment.second.lastReadsIn.begin(), attachment.second.lastReadsIn.end()
            , [cmdBuffer](const GraphicsResource* cmd)
            {
                return cmd == cmdBuffer;
            }
        );
        attachment.second.lastReadsIn.erase(newEnd, attachment.second.lastReadsIn.end());
    }
}

void VulkanResourcesTracker::clearUnwanted()
{
    std::unordered_set<const GraphicsResource*> memResources;
    {
        std::vector<GraphicsResource*> memRes;
        MemoryResource::staticType()->allRegisteredResources(memRes, true);
        memResources.insert(memRes.cbegin(), memRes.cend());
    }
    for (std::map<const MemoryResource*, ResourceAccessors>::iterator itr = resourcesAccessors.begin(); itr != resourcesAccessors.end(); )
    {
        if (memResources.find(itr->first) == memResources.end())
        {
            itr = resourcesAccessors.erase(itr);
        }
        else 
        {
            if (itr->second.lastReadsIn.size() > 1)
            {
                // Since we need to preserve first read alone
                const GraphicsResource* firstRead = itr->second.lastReadsIn[0];
                std::unordered_set<const GraphicsResource*> uniqueReads;
                uniqueReads.insert(firstRead);

                auto newEnd = std::remove_if(itr->second.lastReadsIn.begin(), itr->second.lastReadsIn.end()
                    , [&uniqueReads](const GraphicsResource* res)
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

    for (std::map<const ImageResource*, ResourceAccessors>::iterator itr = renderpassAttachments.begin(); itr != renderpassAttachments.end(); )
    {
        if (memResources.find(itr->first) == memResources.end())
        {
            itr = renderpassAttachments.erase(itr);
        }
        else
        {
            if (itr->second.lastReadsIn.size() > 1)
            {
                // Since we need to preserve first read alone
                const GraphicsResource* firstRead = itr->second.lastReadsIn[0];
                std::unordered_set<const GraphicsResource*> uniqueReads;
                uniqueReads.insert(firstRead);

                auto newEnd = std::remove_if(itr->second.lastReadsIn.begin(), itr->second.lastReadsIn.end()
                    , [&uniqueReads](const GraphicsResource* res)
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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readOnlyBuffers(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;
    ResourceAccessors& accessors = resourcesAccessors[resource.first];
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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readOnlyImages(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;
    ResourceAccessors& accessors = resourcesAccessors[resource.first];
    if (!accessors.lastWrite)
    {
        accessors.lastReadsIn.emplace_back(cmdBuffer);
        accessors.allReadStages |= resource.second;
        accessors.lastReadStages = resource.second;
        return outBarrierInfo;
    }

    // If never read after last write, then layout needs transition before this read no matter write is in this cmd or others
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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readOnlyTexels(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readFromWriteBuffers(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readFromWriteImages(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return readOnlyImages(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::readFromWriteTexels(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return readOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeReadOnlyBuffers(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    fatalAssert(PlatformFunctions::getSetBitCount(resource.second) == 1, "%s: Writing to buffer in several pipeline stages is incorrect", __func__);

    std::optional<ResourceBarrierInfo> outBarrierInfo;
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits(resource.second);
    ResourceAccessors& accessors = resourcesAccessors[resource.first];
    // If never read or write
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;
        return outBarrierInfo;
    }

    // If we are already reading in this cmd buffer then all other steps are already done so wait for just read to finish
    if (std::find(accessors.lastReadsIn.cbegin(), accessors.lastReadsIn.cend(), cmdBuffer) != accessors.lastReadsIn.cend())
    {
        // #TODO(Jeslas): Check if cmd not waiting on other reads is an issue here?
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
        for (const GraphicsResource* cmdBuffer : accessors.lastReadsIn)
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ cmdBuffer, resource.second });

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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeReadOnlyImages(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    fatalAssert(PlatformFunctions::getSetBitCount(resource.second) == 1, "%s: Writing to image in several pipeline stages is incorrect", __func__);

    std::optional<ResourceBarrierInfo> outBarrierInfo;
    VkPipelineStageFlagBits stageFlag = VkPipelineStageFlagBits(resource.second);
    ResourceAccessors& accessors = resourcesAccessors[resource.first];
    // If never read or write
    if (!accessors.lastWrite && accessors.lastReadsIn.empty())
    {
        // Since image layout for Read/writes img depends on caller, use empty read write case to handle it
        ResourceBarrierInfo barrier;

        accessors.lastWrite = cmdBuffer;
        accessors.lastWriteStage = stageFlag;

        outBarrierInfo = barrier;
        return outBarrierInfo;
    }

    // If we are already reading in this cmd buffer then all other steps are already done so wait for just read to finish
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

    if (!accessors.lastReadsIn.empty()) // If not empty then there is other cmds that are reading so wait for those cmds, and transfer layout
    {
        for (const GraphicsResource* cmdBuffer : accessors.lastReadsIn)
            cmdWaitInfo[cmdBuffer].emplace_back(CommandResUsageInfo{ cmdBuffer, resource.second });

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

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeReadOnlyTexels(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeBuffers(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeImages(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return writeReadOnlyImages(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::writeTexels(const GraphicsResource* cmdBuffer
    , const std::pair<const MemoryResource*, VkPipelineStageFlags>& resource)
{
    return writeReadOnlyBuffers(cmdBuffer, resource);
}

std::optional<VulkanResourcesTracker::ResourceBarrierInfo> VulkanResourcesTracker::imageToGeneralLayout(const GraphicsResource* cmdBuffer, const ImageResource* resource)
{
    std::optional<ResourceBarrierInfo> outBarrierInfo;

    auto accessorsItr = resourcesAccessors.find(resource);
    if(accessorsItr != resourcesAccessors.end())
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