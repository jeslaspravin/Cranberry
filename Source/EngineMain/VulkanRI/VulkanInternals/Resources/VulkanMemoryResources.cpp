#include "VulkanMemoryResources.h"
#include "../../VulkanGraphicsHelper.h"
#include "../VulkanDevice.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../../Core/Math/Math.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanBufferResource, VK_OBJECT_TYPE_BUFFER)

void VulkanBufferResource::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanBufferResource::reinitResources()
{
    release();
    BaseType::reinitResources();
    if (getResourceSize() == 0)
    {
        Logger::error("VulkanBufferResource", "%s() : Invalid resource %s", __func__, getObjectName().getChar());
        return;
    }

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(graphicsInstance);

    BUFFER_CREATE_INFO(bufferCreateInfo);
    bufferCreateInfo.size = requiredSize();
    bufferCreateInfo.usage = bufferUsage;

    VkBuffer nextBuffer = VulkanGraphicsHelper::createBuffer(graphicsInstance, bufferCreateInfo, dataFormat);

    if (nextBuffer)
    {
        buffer = nextBuffer;
        graphicsDebugger->markObject(this);
        fatalAssert(VulkanGraphicsHelper::allocateBufferResource(graphicsInstance, this, isStagingResource()),
            "Memory allocation failed for resource");
    }
    else
    {
        Logger::error("VulkanBufferResource", "%s() : Failed creating buffer %s", __func__, getObjectName().getChar());
    }
}

void VulkanBufferResource::release()
{
    if (buffer)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
        for (const std::pair<const BufferViewInfo, VkBufferView>& bufferView : createdBufferViews)
        {
            VulkanGraphicsHelper::destroyBufferView(graphicsInstance, bufferView.second);
        }
        createdBufferViews.clear();
        VulkanGraphicsHelper::deallocateBufferResource(graphicsInstance, this);
        VulkanGraphicsHelper::destroyBuffer(graphicsInstance, buffer);
        buffer = nullptr;
    }
    BaseType::release();
}

String VulkanBufferResource::getObjectName() const
{
    return getResourceName();
}

uint64 VulkanBufferResource::getDispatchableHandle() const
{
    return (uint64)buffer;
}

uint64 VulkanBufferResource::requiredSize() const 
{
    return getResourceSize();
}

bool VulkanBufferResource::canAllocateMemory() const
{
    return buffer && requiredSize() > 0;
}

bool VulkanBufferResource::isValid()
{
    return buffer != nullptr;
}

VkBufferView VulkanBufferResource::createBufferView(const BufferViewInfo& viewInfo)
{
    BUFFER_VIEW_CREATE_INFO(bufferViewCreateInfo);
    bufferViewCreateInfo.buffer = buffer;
    bufferViewCreateInfo.format = (VkFormat)EPixelDataFormat::getFormatInfo(dataFormat)->format;
    bufferViewCreateInfo.offset = viewInfo.startOffset;
    bufferViewCreateInfo.range = viewInfo.size;

    return VulkanGraphicsHelper::createBufferView(gEngine->getRenderApi()->getGraphicsInstance(), bufferViewCreateInfo);
}

VkBufferView VulkanBufferResource::getBufferView(const BufferViewInfo& viewInfo)
{
    if (!isValid() || EPixelDataFormat::Undefined == dataFormat)
    {
        return nullptr;
    }

    VkBufferView bufferView = nullptr;
    const auto& foundItr = createdBufferViews.find(viewInfo);
    if (foundItr == createdBufferViews.cend())
    {
        bufferView = createBufferView(viewInfo);
        if (bufferView != nullptr)
        {
            createdBufferViews[viewInfo] = bufferView;
        }
    }
    else
    {
        bufferView = foundItr->second;
    }
    return bufferView;
}

//////////////////////////////////////////////////////////////////////////
//// Image Resources 
//////////////////////////////////////////////////////////////////////////


DEFINE_VK_GRAPHICS_RESOURCE(VulkanImageResource, VK_OBJECT_TYPE_IMAGE)

VulkanImageResource::VulkanImageResource(EPixelDataFormat::Type imageFormat, bool cpuAccessible /*= false*/)
    : ImageResource(imageFormat)
    , defaultImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    , defaultFeaturesRequired(VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
    , createFlags(0)
    , tiling(VK_IMAGE_TILING_OPTIMAL)
    , type(VK_IMAGE_TYPE_2D)
    , viewType(VkImageViewType::VK_IMAGE_VIEW_TYPE_2D)
{
    if (cpuAccessible)
    {
        tiling = VK_IMAGE_TILING_LINEAR;
        bIsStagingResource = true;
    }
}

VulkanImageResource::VulkanImageResource()
    : ImageResource(EPixelDataFormat::ABGR8_UI32_Packed)
    , defaultImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    , defaultFeaturesRequired(VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
    , createFlags(0)
    , tiling(VK_IMAGE_TILING_OPTIMAL)
    , type(VK_IMAGE_TYPE_2D)
    , viewType(VkImageViewType::VK_IMAGE_VIEW_TYPE_2D)
{}

void VulkanImageResource::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanImageResource::reinitResources()
{
    release();
    BaseType::reinitResources();
    VkImageUsageFlags imageUsage = defaultImageUsage;
    VkFormatFeatureFlags featuresRequired = defaultFeaturesRequired;

    if (isRenderTarget)
    {
        imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        featuresRequired = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

        if (!formatInfo)
        {
            Logger::error("VulkanImageResource", "%s() : Not supported image format", __func__);
            return;
        }
        if (EPixelDataFormat::isDepthFormat(dataFormat))
        {
            imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            featuresRequired |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else
        {
            imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            featuresRequired |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
        }
        // In case of using same target as both Render target and shader sampled image
        imageUsage |= ((shaderUsage & EImageShaderUsage::Sampling) > 0) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
        featuresRequired |= ((shaderUsage & EImageShaderUsage::Sampling) > 0) ? VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT : 0;
        tiling = VK_IMAGE_TILING_OPTIMAL;
        // In render targets only one mip map is allowed
        numOfMips = 1;
    }
    else
    {
        if (numOfMips == 0)
        {
            // TODO (Jeslas) : Check if 1D or 3D can have more mips and render targets
            numOfMips = mipCountFromDim();
        }
        if (type != VK_IMAGE_TYPE_2D)
        {
            numOfMips = 1;
            sampleCounts = EPixelSampleCount::SampleCount1;
        }
        if(sampleCounts != EPixelSampleCount::SampleCount1)
        {
            numOfMips = 1;
        }
        if ((createFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) > 0 && layerCount < 6)
        {
            Logger::warn("VulkanImageResource", "%s() : Cube map image should have at least 6 layers, current layer count %d", __func__, layerCount);
            layerCount = 6;
        }

        imageUsage |= ((shaderUsage & EImageShaderUsage::Sampling) > 0) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
        featuresRequired |= ((shaderUsage & EImageShaderUsage::Sampling) > 0) ? VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT : 0;
        imageUsage |= ((shaderUsage & EImageShaderUsage::Writing) > 0) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
        featuresRequired |= ((shaderUsage & EImageShaderUsage::Writing) > 0) ? VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT : 0;

        // TODO(Jeslas) : Revisit for cpu accessible image
        if (tiling == VK_IMAGE_TILING_LINEAR)
        {
            numOfMips = 1;
            layerCount = 1;
            imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            featuresRequired = VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
            sampleCounts = EPixelSampleCount::SampleCount1;
        }
    }


    if (getResourceSize() == 0)
    {
        Logger::error("VulkanImageResource", "%s() : Invalid resource %s", __func__, getObjectName().getChar());
        return;
    }

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(graphicsInstance);

    IMAGE_CREATE_INFO(imgCreateInfo);
    imgCreateInfo.flags = createFlags;
    imgCreateInfo.imageType = type;
    imgCreateInfo.tiling = tiling;
    imgCreateInfo.usage = imageUsage;
    imgCreateInfo.samples = (VkSampleCountFlagBits)sampleCounts;
    imgCreateInfo.mipLevels = numOfMips;
    imgCreateInfo.format = (VkFormat)getFormatInfo(dataFormat)->format;
    imgCreateInfo.arrayLayers = layerCount;
    imgCreateInfo.extent = { dimensions.x, dimensions.y, dimensions.z };
    imgCreateInfo.initialLayout = tiling == VK_IMAGE_TILING_LINEAR? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage nextImage = VulkanGraphicsHelper::createImage(graphicsInstance, imgCreateInfo, featuresRequired);
    setLayerCount(imgCreateInfo.arrayLayers);
    setNumOfMips(imgCreateInfo.mipLevels);

    if (nextImage)
    {
        image = nextImage;
        graphicsDebugger->markObject(this);
        fatalAssert(VulkanGraphicsHelper::allocateImageResource(graphicsInstance, this, isStagingResource()),
            "Memory allocation failed for resource");
    }
    else
    {
        Logger::error("VulkanImageResource", "%s() : Failed creating image %s", __func__, getObjectName().getChar());
    }
}

void VulkanImageResource::release()
{
    if (image)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
        for (const std::pair<const ImageViewInfo, VkImageView>& imageView : createdImageViews)
        {
            VulkanGraphicsHelper::destroyImageView(graphicsInstance, imageView.second);
        }
        createdImageViews.clear();
        VulkanGraphicsHelper::deallocateImageResource(graphicsInstance, this);
        VulkanGraphicsHelper::destroyImage(graphicsInstance, image);
        image = nullptr;
    }
    BaseType::release();
}

uint64 VulkanImageResource::getResourceSize() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        // TODO(Jeslas) : Check if layer count is necessary in this case
        return dimensions.x * dimensions.y * dimensions.z * layerCount * formatInfo->pixelDataSize;
    }
    return 0;
}

bool VulkanImageResource::isValid()
{
    return image != nullptr;
}

String VulkanImageResource::getObjectName() const
{
    return getResourceName();
}

uint64 VulkanImageResource::getDispatchableHandle() const
{
    return (uint64)image;
}

uint64 VulkanImageResource::requiredSize() const 
{
    return getResourceSize();
}

bool VulkanImageResource::canAllocateMemory() const
{
    return image && requiredSize() > 0;
}

VkImageView VulkanImageResource::createImageView(const ImageViewInfo& viewInfo)
{
    VkImageAspectFlags viewAspects = 0;
    if (EPixelDataFormat::isDepthFormat(dataFormat))
    {
        viewAspects = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewAspects |= viewInfo.bUseStencil && EPixelDataFormat::isStencilFormat(dataFormat) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
    }
    else
    {
        viewAspects = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    IMAGE_VIEW_CREATE_INFO(imageViewCreateInfo);
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.format = (VkFormat)EPixelDataFormat::getFormatInfo(dataFormat)->format;
    imageViewCreateInfo.viewType = viewType;
    imageViewCreateInfo.subresourceRange = {
        viewAspects,
        viewInfo.viewSubresource.baseMip,
        viewInfo.viewSubresource.mipCount,
        viewInfo.viewSubresource.baseLayer,
        viewInfo.viewSubresource.layersCount
    };
    imageViewCreateInfo.components = {
        (VkComponentSwizzle)EPixelComponentMapping::getComponentMapping(viewInfo.componentMapping.r)->mapping,
        (VkComponentSwizzle)EPixelComponentMapping::getComponentMapping(viewInfo.componentMapping.g)->mapping,
        (VkComponentSwizzle)EPixelComponentMapping::getComponentMapping(viewInfo.componentMapping.b)->mapping,
        (VkComponentSwizzle)EPixelComponentMapping::getComponentMapping(viewInfo.componentMapping.a)->mapping,
    };

    return VulkanGraphicsHelper::createImageView(gEngine->getRenderApi()->getGraphicsInstance(), imageViewCreateInfo);
}

VkImageView VulkanImageResource::getImageView(const ImageViewInfo& viewInfo)
{
    if (!isValid())
    {
        return nullptr;
    }

    VkImageView imageView = nullptr;
    const auto& foundItr = createdImageViews.find(viewInfo);
    if (foundItr == createdImageViews.cend())
    {
        imageView = createImageView(viewInfo);
        if (imageView != nullptr)
        {
            createdImageViews[viewInfo] = imageView;
        }
    }
    else
    {
        imageView = foundItr->second;
    }
    return imageView;
}
