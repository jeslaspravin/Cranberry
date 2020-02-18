#pragma once
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/QueueResource.h"
#include <map>
#include "vulkan_core.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSyncResource.h"

struct QueueCommandPool
{
    VkCommandPool tempCommandsPool;
    VkCommandPool resetableCommandPool;
    VkCommandPool oneTimeRecordPool;
};

class ExperimentalEngine : public GameEngine
{
    class VulkanDevice* vDevice;
    VkDevice device;
    std::vector<QueueResourceBase*>* deviceQueues;
    const class VulkanDebugGraphics* graphicsDbg;

    std::map<EQueueFunction, QueueCommandPool> pools;
    VkCommandBuffer swapchainCmdBuffer;
    SharedPtr<GraphicsFence> vFence;
    void createPools();
    void destroyPools();
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

};