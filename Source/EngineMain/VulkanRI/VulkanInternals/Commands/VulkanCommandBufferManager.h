#pragma once

#include "../../../RenderInterface/Resources/GraphicsResources.h"
#include "../../Resources/IVulkanResources.h"
#include "../../../Core/String/String.h"
#include "../VulkanMacros.h"
#include "../../../RenderInterface/Resources/QueueResource.h"
#include "../../../RenderInterface/Rendering/CommandBuffer.h"

#include <map>

class QueueResourceBase;
class VulkanDevice;
class GraphicsSemaphore;
class GraphicsFence;
struct CommandSubmitInfo;

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
};

class VulkanCmdBufferManager
{
private:
    std::map<EQueueFunction, VulkanCommandPool> pools;
    // Just a pointer to pool in the pools map
    VulkanCommandPool* genericPool;
    // Map of command buffers to its names that are currently available, Temp buffers wont be stored here as they are freed after usage
    std::map<String, VulkanCmdBufferState> commandBuffers;

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

    void endCmdBuffer(const GraphicsResource* cmdBuffer);
    void cmdFinished(const GraphicsResource* cmdBuffer);
    void cmdFinished(const String& cmdName);
    void freeCmdBuffer(const GraphicsResource* cmdBuffer);

    VkCommandBuffer getRawBuffer(const GraphicsResource* cmdBuffer) const;
    const GraphicsResource* getCmdBuffer(const String& cmdName) const;
    uint32 getQueueFamilyIdx(const GraphicsResource* cmdBuffer) const;
    uint32 getQueueFamilyIdx(EQueueFunction queue) const;
    ECmdState getState(const GraphicsResource* cmdBuffer) const;

    //************************************
    // Method:    submitCmds - Currently all commands being submitted must be from same queue
    // FullName:  VulkanCmdBufferManager::submitCmds
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const std::vector<VulkanSubmitInfo> & commands - List of commands to be submitted
    // Parameter: GraphicsFence * cmdsCompleteFence - Fence that gets signalled when all of the commands submitted are complete
    //************************************
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo>& commands, GraphicsFence* cmdsCompleteFence);

    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& command, GraphicsFence* cmdsCompleteFence);
};