#pragma once
#include "../../../RenderInterface/Resources/MemoryResources.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"
#include "../../../Core/String/String.h"

#include <unordered_map>

class VulkanDevice;

class VulkanBufferResource : public BufferResource, public IVulkanMemoryResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanBufferResource,,BufferResource,)
    
protected:
    // Always buffer can be copied from and copied to
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    std::unordered_map<BufferViewInfo, VkBufferView> createdBufferViews;

public:
    VkBuffer buffer;

protected:
    VkBufferView createBufferView(const BufferViewInfo& viewInfo);
public:
    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    /* End - GraphicsResource implementations */

    /* MemoryResource implementations */
    bool isValid() override;
    /* End - MemoryResource implementations */

    /* IVulkanResources implementations */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    uint64 requiredSize() const override;
    bool canAllocateMemory() const override;
    /* End - IVulkanResources implementations */

    VkBufferView getBufferView(const BufferViewInfo& viewInfo);
};


class VulkanImageResource : public ImageResource, public IVulkanMemoryResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanImageResource, , ImageResource, )

protected:

    VkImageUsageFlags defaultImageUsage;
    VkFormatFeatureFlags defaultFeaturesRequired;
    VkImageCreateFlags createFlags;
    VkImageTiling tiling;
    VkImageType type;

    std::unordered_map<ImageViewInfo, VkImageView> createdImageViews;
    VkImageViewType viewType;

    VulkanImageResource();
public:
    VkImage image;

protected:
    VkImageView createImageView(const ImageViewInfo& viewInfo);
public:
    VulkanImageResource(EPixelDataFormat::Type imageFormat,bool cpuAccessible = false);

    /* GraphicsResource implementations */
    void init() override;
    void reinitResources() override;
    void release() override;
    uint64 getResourceSize() const override;
    /* End - GraphicsResource implementations */

    /* MemoryResource implementations */
    bool isValid() override;
    /* End - MemoryResource implementations */

    /* IVulkanResources implementations */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;
    uint64 requiredSize()  const override;
    bool canAllocateMemory() const override;
    /* End - IVulkanResources implementations */

    VkImageView getImageView(const ImageViewInfo& viewInfo);
};