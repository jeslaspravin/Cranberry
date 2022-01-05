/*!
 * \file VulkanCommandBufferManager.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "RenderInterface/Resources/MemoryResources.h"
#include "Types/Containers/MinAllocVector.h"
#include "VulkanInternals/Resources/IVulkanResources.h"
#include "String/String.h"
#include "VulkanInternals/VulkanMacros.h"
#include "RenderInterface/Resources/QueueResource.h"
#include "RenderInterface/Rendering/CommandBuffer.h"

#include <map>

class QueueResourceBase;
class VulkanDevice;
class GraphicsSemaphore;
class GraphicsFence;
struct CommandSubmitInfo;
class VulkanResourcesTracker;

namespace std
{
    template <class _Ty>
    class optional;
}

struct VulkanCommandPoolInfo
{
    VulkanDevice* vDevice;
    VkDevice logicalDevice;
    uint32 vulkanQueueIndex;
    QueueResourceBase* queueResource = nullptr;
    EQueueFunction queueType;
};

class VulkanCommandPool : public GraphicsResource, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCommandPool, , GraphicsResource, );
private:
    friend class VulkanCmdBufferManager;

    VkCommandPool tempCommandsPool;
    VkCommandPool rerecordableCommandPool;
    VkCommandPool oneTimeRecordPool;

    String poolName;
    VulkanCommandPoolInfo cmdPoolInfo;
public:
    /* GraphicsResource overrides */
    void init() override;
    void reinitResources() override;
    void release() override;
    String getResourceName() const override;
    void setResourceName(const String& name) override;
    /* IVulkanResources overrides */
    String getObjectName() const override;
    /* Override ends */

    VkCommandPool getCommandPool(class VulkanCommandBuffer const* cmdBuffer) const;
};

struct VulkanCmdBufferState
{
    class VulkanCommandBuffer* cmdBuffer;
    ECmdState cmdState = ECmdState::Idle;
    // Will be valid after submit
    int32 cmdSyncInfoIdx = -1;
};

struct VulkanCmdSubmitSyncInfo
{
    bool bIsAdvancedSubmit = false;
    uint32 refCount = 0;
    FenceRef completeFence;
    SemaphoreRef signalingSemaphore;
};

class VulkanCmdBufferManager
{
private:
    std::map<EQueueFunction, VulkanCommandPool> pools;
    // Just a pointer to pool in the pools map
    VulkanCommandPool* genericPool;
    // Map of command buffers to its names that are currently available, Temp buffers wont be stored here as they are freed after usage
    std::map<String, VulkanCmdBufferState> commandBuffers;
    MinAllocVector<VulkanCmdSubmitSyncInfo> cmdsSyncInfo;

    VulkanDevice* vDevice;

private:
    void createPools();
    VulkanCommandPool& getPool(EQueueFunction forQueue);
    VkQueue getVkQueue(EQueuePriority::Enum priority, QueueResourceBase* queueRes);
public:
    VulkanCmdBufferManager(class VulkanDevice* vulkanDevice);
    ~VulkanCmdBufferManager();

    const GraphicsResource* beginTempCmdBuffer(const String& cmdName, EQueueFunction usingQueue);
    const GraphicsResource* beginRecordOnceCmdBuffer(const String& cmdName, EQueueFunction usingQueue);
    const GraphicsResource* beginReuseCmdBuffer(const String& cmdName, EQueueFunction usingQueue);

    void startRenderPass(const GraphicsResource* cmdBuffer);
    bool isInRenderPass(const GraphicsResource* cmdBuffer) const;
    void endRenderPass(const GraphicsResource* cmdBuffer);

    void endCmdBuffer(const GraphicsResource* cmdBuffer);
    void cmdFinished(const GraphicsResource* cmdBuffer, VulkanResourcesTracker* resourceTracker);
    void cmdFinished(const String& cmdName, VulkanResourcesTracker* resourceTracker);
    void finishAllSubmited(VulkanResourcesTracker* resourceTracker);
    void freeCmdBuffer(const GraphicsResource* cmdBuffer);

    VkCommandBuffer getRawBuffer(const GraphicsResource* cmdBuffer) const;
    const GraphicsResource* getCmdBuffer(const String& cmdName) const;
    uint32 getQueueFamilyIdx(const GraphicsResource* cmdBuffer) const;
    uint32 getQueueFamilyIdx(EQueueFunction queue) const;
    ECmdState getState(const GraphicsResource* cmdBuffer) const;
    SemaphoreRef cmdSignalSemaphore(const GraphicsResource* cmdBuffer) const;

    bool isComputeCmdBuffer(const GraphicsResource* cmdBuffer) const;
    bool isGraphicsCmdBuffer(const GraphicsResource* cmdBuffer) const;
    bool isTransferCmdBuffer(const GraphicsResource* cmdBuffer) const;
    //************************************
    // Method:    submitCmds - Currently all commands being submitted must be from same queue
    // FullName:  VulkanCmdBufferManager::submitCmds
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const std::vector<VulkanSubmitInfo> & commands - List of commands to be submitted
    // Parameter: GraphicsFence * cmdsCompleteFence - Fence that gets signalled when all of the commands submitted are complete
    //************************************
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo>& commands, FenceRef& cmdsCompleteFence);
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& command, FenceRef& cmdsCompleteFence);

    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands, VulkanResourcesTracker* resourceTracker);
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command, VulkanResourcesTracker* resourceTracker);
};

/// <summary>
/// VulkanResourcesTracker - Tracks resources used in commands
/// </summary>
class VulkanResourcesTracker
{
private:
    struct ResourceAccessors
    {
        // Last reads after last writes
        std::vector<const GraphicsResource*> lastReadsIn;
        VkPipelineStageFlags allReadStages = 0;
        // Useful to resolve image old layout in case of multiple reads
        VkPipelineStageFlags lastReadStages = 0;
        const GraphicsResource* lastWrite = nullptr;
        VkPipelineStageFlagBits lastWriteStage;
    };

public:
    struct CommandResUsageInfo
    {
        const GraphicsResource* cmdBuffer = nullptr;
        VkPipelineStageFlags usageStages;
    };

    struct ResourceBarrierInfo
    {
        MemoryResourceRef resource = nullptr;
        ResourceAccessors accessors;
    };

private:

    std::map<MemoryResourceRef, ResourceAccessors> resourcesAccessors;
    // Render pass attachments auto transfer layouts so it will need to put barrier only after transfer
    std::map<const ImageResourceRef, ResourceAccessors> renderpassAttachments;

    using CmdWaitInfoMap = std::map<const GraphicsResource*, std::vector<CommandResUsageInfo>>;
    CmdWaitInfoMap cmdWaitInfo;

public:
    const std::vector<CommandResUsageInfo>* getCmdBufferDeps(const GraphicsResource* cmdBuffer) const;
    void clearCmdBufferDeps(const GraphicsResource* cmdBuffer);
    void clearFinishedCmd(const GraphicsResource* cmdBuffer);
    void clearUnwanted();

    /* Reading resources functions */
    std::optional<ResourceBarrierInfo> readOnlyBuffers(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> readOnlyImages(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> readOnlyTexels(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> readFromWriteBuffers(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> readFromWriteImages(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> readFromWriteTexels(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);

    /* Writing resources functions */
    std::optional<ResourceBarrierInfo> writeReadOnlyBuffers(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> writeReadOnlyImages(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> writeReadOnlyTexels(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> writeBuffers(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> writeImages(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);
    std::optional<ResourceBarrierInfo> writeTexels(const GraphicsResource* cmdBuffer
        , const std::pair<MemoryResourceRef, VkPipelineStageFlags>& resource);

    std::optional<ResourceBarrierInfo> imageToGeneralLayout(const GraphicsResource* cmdBuffer, const ImageResourceRef resource);
};