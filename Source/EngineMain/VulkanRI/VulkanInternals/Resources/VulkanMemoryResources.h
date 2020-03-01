#pragma once
#include "../../../RenderInterface/Resources/MemoryResources.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"
#include "../../../Core/String/String.h"

class VulkanDevice;

class VulkanBufferResource : public BufferResource, public IVulkanMemoryResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanBufferResource,,BufferResource,)

private:
    String bufferName;

protected:
    // Always buffer can be copied from and copied to
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

public:
    VkBuffer buffer;

    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    String getResourceName() const override;
    /* End - GraphicsResource implementations */
    /* IVulkanResources implementations */
    String getObjectName() const override;
    void setObjectName(const String& name) override;
    uint64 getDispatchableHandle() const override;
    uint64 requiredSize() override;
    bool canAllocateMemory() override;
    /* End - IVulkanResources implementations */
};
