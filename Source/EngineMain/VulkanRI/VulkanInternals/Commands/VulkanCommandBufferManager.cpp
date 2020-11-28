#include "VulkanCommandBufferManager.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../Resources/VulkanQueueResource.h"
#include "../VulkanDevice.h"
#include "../Resources/VulkanSyncResource.h"

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
        cmdBufferItr->second.cmdState = ECmdState::Recording;
    }

    CMD_BUFFER_BEGIN_INFO(cmdBuffBeginInfo);
    cmdBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(cmdBuffer->cmdBuffer, &cmdBuffBeginInfo);
    return cmdBuffer;
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

void VulkanCmdBufferManager::cmdFinished(const GraphicsResource* cmdBuffer)
{
    const auto* vCmdBuffer = static_cast<const VulkanCommandBuffer*>(cmdBuffer);

    if (!vCmdBuffer->bIsTempBuffer)
    {
        commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Recorded;
    }
}

void VulkanCmdBufferManager::cmdFinished(const String& cmdName)
{
    auto cmdBufferItr = commandBuffers.find(cmdName);
    if (cmdBufferItr != commandBuffers.end())
    {
        cmdBufferItr->second.cmdState = ECmdState::Recorded;
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

void VulkanCmdBufferManager::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo>& commands, GraphicsFence* cmdsCompleteFence)
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
        std::vector<VkSemaphore>& signallingSemaphores = allSignallingSemaphores[cmdSubmitIdx];
        signallingSemaphores.resize(commands[cmdSubmitIdx].signalSemaphores.size());

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
            waitOnSemaphores[i] = static_cast<VulkanSemaphore*>(commands[cmdSubmitIdx].waitOn[i].waitOnSemaphore)->semaphore;
            waitingStages[i] = commands[cmdSubmitIdx].waitOn[i].stagesThatWaits;
        }
        for (int32 i = 0; i < commands[cmdSubmitIdx].signalSemaphores.size(); ++i)
        {
            signallingSemaphores[i] = static_cast<VulkanSemaphore*>(commands[cmdSubmitIdx].signalSemaphores[i])->semaphore;
        }

        allSubmitInfo[cmdSubmitIdx].commandBufferCount = uint32(cmdBuffers.size());
        allSubmitInfo[cmdSubmitIdx].pCommandBuffers = cmdBuffers.data();
        allSubmitInfo[cmdSubmitIdx].signalSemaphoreCount = uint32(signallingSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pSignalSemaphores = signallingSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].waitSemaphoreCount = uint32(waitOnSemaphores.size());
        allSubmitInfo[cmdSubmitIdx].pWaitSemaphores = waitOnSemaphores.data();
        allSubmitInfo[cmdSubmitIdx].pWaitDstStageMask = waitingStages.data();
    }

    VkQueue vQueue = getVkQueue(priority, queueRes);
    fatalAssert(vDevice->vkQueueSubmit(vQueue, uint32(allSubmitInfo.size()), allSubmitInfo.data(), (cmdsCompleteFence == nullptr)
        ? nullptr : static_cast<VulkanFence*>(cmdsCompleteFence)->fence) == VK_SUCCESS
        , "Failed submitting commands to queue");

    for (const CommandSubmitInfo& command : commands)
    {
        for (const GraphicsResource* cmdBuffer : command.cmdBuffers)
        {
            if (!static_cast<const VulkanCommandBuffer*>(cmdBuffer)->bIsTempBuffer)
            {
                commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
            }
        }
    }
}

void VulkanCmdBufferManager::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& command, GraphicsFence* cmdsCompleteFence)
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
        waitOnSemaphores[i] = static_cast<VulkanSemaphore*>(command.waitOn[i].waitOnSemaphore)->semaphore;
        waitingStages[i] = command.waitOn[i].stagesThatWaits;
    }
    for (int32 i = 0; i < command.signalSemaphores.size(); ++i)
    {
        signallingSemaphores[i] = static_cast<VulkanSemaphore*>(command.signalSemaphores[i])->semaphore;
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
    fatalAssert(vDevice->vkQueueSubmit(vQueue, 1, &cmdSubmitInfo, (cmdsCompleteFence == nullptr)
        ? nullptr : static_cast<VulkanFence*>(cmdsCompleteFence)->fence) == VK_SUCCESS
        , "Failed submitting command to queue");

    for (const GraphicsResource* cmdBuffer : command.cmdBuffers)
    {
        if (!static_cast<const VulkanCommandBuffer*>(cmdBuffer)->bIsTempBuffer)
        {
            commandBuffers[cmdBuffer->getResourceName()].cmdState = ECmdState::Submitted;
        }
    }
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
