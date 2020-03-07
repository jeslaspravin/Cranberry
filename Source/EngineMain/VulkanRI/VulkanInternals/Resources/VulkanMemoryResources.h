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
    uint64 requiredSize() const override;
    bool canAllocateMemory() const override;
    /* End - IVulkanResources implementations */
};


class VulkanImageResource : public ImageResource, public IVulkanMemoryResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanImageResource, , BufferResource, )

private:
    String imageName;

protected:

    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkFormatFeatureFlags featureRequired = VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    VkImageCreateFlags createFlags = 0;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageType type = VK_IMAGE_TYPE_2D;

    VulkanImageResource() : ImageResource(EPixelDataFormat::RGBA_U8_Packed) {}
public:
    VkImage image;

    VulkanImageResource(EPixelDataFormat::Type imageFormat,bool cpuAccessible = false);

    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    String getResourceName() const override;
    uint64 getResourceSize() const override;
    /* End - GraphicsResource implementations */
    /* IVulkanResources implementations */
    String getObjectName() const override;
    void setObjectName(const String& name) override;
    uint64 getDispatchableHandle() const override;
    uint64 requiredSize()  const override;
    bool canAllocateMemory() const override;
    /* End - IVulkanResources implementations */
};