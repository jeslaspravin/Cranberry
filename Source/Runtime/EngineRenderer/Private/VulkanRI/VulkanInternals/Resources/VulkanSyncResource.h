#pragma once


#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "VulkanRI/Resources/IVulkanResources.h"
#include "VulkanRI/VulkanInternals/VulkanMacros.h"
#include "String/String.h"

#include <vulkan_core.h>

class VulkanDevice;

class VulkanSemaphore final : public GraphicsSemaphore,public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanSemaphore,,GraphicsSemaphore,)

private:
    const VulkanDevice* vulkanDevice;
    VkDevice ownerDevice;    
    VulkanSemaphore() = default;

public:
    VkSemaphore semaphore;

    VulkanSemaphore(const VulkanDevice* deviceInstance);

    void waitForSignal() const override;
    bool isSignaled() const override;
    void resetSignal() override;
    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    /* End - GraphicsResource implementations */
    /* IVulkanResources implementations */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    /* End - IVulkanResources implementations */
};

class VulkanTimelineSemaphore final : public GraphicsTimelineSemaphore, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanTimelineSemaphore, , GraphicsTimelineSemaphore, )

private:

    const VulkanDevice* vulkanDevice;
    VkDevice ownerDevice;
    VulkanTimelineSemaphore() = default;

public:
    VkSemaphore semaphore;

    VulkanTimelineSemaphore(const VulkanDevice* deviceInstance);

    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    /* End - GraphicsResource implementations */
    void waitForSignal(uint64 value) const override;
    bool isSignaled(uint64 value) const override;
    void resetSignal(uint64 value) override;
    uint64 currentValue() const override;

    /* IVulkanResources implementations */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    /* End - IVulkanResources implementations */
};

class VulkanFence final : public GraphicsFence, public IVulkanResources
{

    DECLARE_VK_GRAPHICS_RESOURCE(VulkanFence, , GraphicsFence, )

private:

    const VulkanDevice* vulkanDevice;
    VkDevice ownerDevice;
    VulkanFence() = default;

    bool bCreateSignaled;
public:
    VkFence fence;

    VulkanFence(const VulkanDevice* deviceInstance,bool bIsSignaled);

    void waitForSignal() const override;
    bool isSignaled() const override;
    void resetSignal() override;
    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    /* End - GraphicsResource implementations */

    /* IVulkanResources implementations */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    /* End - IVulkanResources implementations */
};