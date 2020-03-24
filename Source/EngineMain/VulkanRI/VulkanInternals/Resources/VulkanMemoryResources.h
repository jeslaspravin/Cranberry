#pragma once
#include "../../../RenderInterface/Resources/MemoryResources.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"
#include "../../../Core/String/String.h"

class VulkanDevice;

class VulkanBufferResource : public BufferResource, public IVulkanMemoryResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanBufferResource,,BufferResource,)
    
protected:
    // Always buffer can be copied from and copied to
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

public:
    VkBuffer buffer;

    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    /* End - GraphicsResource implementations */

    /* MemoryResource implementations */
    bool isValid();
    /* End - MemoryResource implementations */

    /* IVulkanResources implementations */
    String getObjectName() const override;
    void setObjectName(const String& name) override;
    uint64 getDispatchableHandle() const override;
    uint64 requiredSize() const override;
    bool canAllocateMemory() const override;
    /* End - IVulkanResources implementations */
};


class VulkanImageResource : public ImageResource, public IVulkanMemoryResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanImageResource, , BufferResource, )

protected:

    VkImageUsageFlags defaultImageUsage;
    VkFormatFeatureFlags defaultFeaturesRequired;
    VkImageCreateFlags createFlags;
    VkImageTiling tiling;
    VkImageType type;

    VulkanImageResource();
public:
    VkImage image;

    VulkanImageResource(EPixelDataFormat::Type imageFormat,bool cpuAccessible = false);

    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    uint64 getResourceSize() const override;
    /* End - GraphicsResource implementations */

    /* MemoryResource implementations */
    bool isValid();
    /* End - MemoryResource implementations */

    /* IVulkanResources implementations */
    String getObjectName() const override;
    void setObjectName(const String& name) override;
    uint64 getDispatchableHandle() const override;
    uint64 requiredSize()  const override;
    bool canAllocateMemory() const override;
    /* End - IVulkanResources implementations */
};