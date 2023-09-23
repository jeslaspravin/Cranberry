/*!
 * \file VulkanMemoryResources.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/Resources/VulkanMemoryResources.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanRHIModule.h"

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
        LOG_ERROR("VulkanBufferResource", "Invalid resource {}", getObjectName().getChar());
        return;
    }

    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const VulkanDebugGraphics *graphicsDebugger = VulkanGraphicsHelper::debugGraphics(graphicsInstance);

    BUFFER_CREATE_INFO(bufferCreateInfo);
    bufferCreateInfo.size = requiredSize();
    bufferCreateInfo.usage = bufferUsage;

    VkBuffer nextBuffer = VulkanGraphicsHelper::createBuffer(graphicsInstance, bufferCreateInfo, dataFormat);

    if (nextBuffer)
    {
        buffer = nextBuffer;
        graphicsDebugger->markObject(this);
        fatalAssertf(
            VulkanGraphicsHelper::allocateBufferResource(graphicsInstance, this, isStagingResource()), "Memory allocation failed for resource"
        );
    }
    else
    {
        LOG_ERROR("VulkanBufferResource", "Failed creating buffer {}", getObjectName().getChar());
    }
}

void VulkanBufferResource::release()
{
    if (buffer)
    {
        IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
        for (const std::pair<const BufferViewInfo, VkBufferView> &bufferView : createdBufferViews)
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

String VulkanBufferResource::getObjectName() const { return getResourceName(); }

uint64 VulkanBufferResource::getDispatchableHandle() const { return (uint64)buffer; }

uint64 VulkanBufferResource::requiredSize() const { return getResourceSize(); }

bool VulkanBufferResource::canAllocateMemory() const { return buffer && requiredSize() > 0; }

bool VulkanBufferResource::isValid() { return buffer != nullptr; }

VkBufferView VulkanBufferResource::createBufferView(const BufferViewInfo &viewInfo)
{
    BUFFER_VIEW_CREATE_INFO(bufferViewCreateInfo);
    bufferViewCreateInfo.buffer = buffer;
    bufferViewCreateInfo.format = EngineToVulkanAPI::vulkanDataFormat(dataFormat);
    bufferViewCreateInfo.offset = viewInfo.startOffset;
    bufferViewCreateInfo.range = viewInfo.size;

    return VulkanGraphicsHelper::createBufferView(IVulkanRHIModule::get()->getGraphicsInstance(), bufferViewCreateInfo);
}

VkBufferView VulkanBufferResource::getBufferView(const BufferViewInfo &viewInfo)
{
    if (!isValid() || EPixelDataFormat::Undefined == dataFormat)
    {
        return nullptr;
    }

    VkBufferView bufferView = nullptr;
    const auto &foundItr = createdBufferViews.find(viewInfo);
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

VulkanImageResource::VulkanImageResource(ImageResourceCreateInfo createInfo, bool cpuAccessible /*= false*/)
    : ImageResource(createInfo)
    , defaultImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    , defaultFeaturesRequired(VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
    , createFlags(0)
    , tiling(VK_IMAGE_TILING_OPTIMAL)
    , type(VK_IMAGE_TYPE_2D)
    , viewType(VkImageViewType::VK_IMAGE_VIEW_TYPE_2D)
    , image(nullptr)
{
    if (cpuAccessible)
    {
        tiling = VK_IMAGE_TILING_LINEAR;
        bIsStagingResource = true;
    }
}

VulkanImageResource::VulkanImageResource()
    : ImageResource()
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
    // If dimension.z is greater than 1 or layer larger than 1, it must be either cube/cube array or 3D
    // or 2D array
    if (dimensions.z > 1 || layerCount > 1)
    {
        if (BIT_SET(createFlags, VkImageCreateFlagBits::VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
        {
            debugAssert(dimensions.z == 1);
            // Then must be cube array
            if (layerCount > 6 && (layerCount % 6 != 0))
            {
                LOG_WARN(
                    "VulkanImageResource",
                    "Cube map image {} should have 6 multiple layers, current layer "
                    "count {}",
                    getResourceName().getChar(), layerCount
                );
                layerCount = ((layerCount / 6) + 1) * 6;
            }
        }
        else
        {
            if (dimensions.z > 1)
            {
                type = VkImageType::VK_IMAGE_TYPE_3D;
                viewType = VK_IMAGE_VIEW_TYPE_3D;
            }
            // https://khronos.org/registry/vulkan/specs/1.2-extensions/html/chap12.html#VUID-VkImageViewCreateInfo-image-04970
            createFlags |= (layerCount == 1) ? VkImageCreateFlagBits::VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT : 0;
        }
    }

    if (isRenderTarget)
    {
        imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        featuresRequired = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

        if (!formatInfo)
        {
            LOG_ERROR("VulkanImageResource", "Not supported image format");
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
            // TODO(Jeslas) : Check if 1D or 3D can have more mips and render targets
            numOfMips = mipCountFromDim();
        }
        if (type != VK_IMAGE_TYPE_2D)
        {
            numOfMips = 1;
            sampleCounts = EPixelSampleCount::SampleCount1;
        }
        if (sampleCounts != EPixelSampleCount::SampleCount1)
        {
            numOfMips = 1;
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
        LOG_ERROR("VulkanImageResource", "Invalid resource {}", getObjectName().getChar());
        return;
    }

    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    const VulkanDebugGraphics *graphicsDebugger = VulkanGraphicsHelper::debugGraphics(graphicsInstance);

    IMAGE_CREATE_INFO(imgCreateInfo);
    imgCreateInfo.flags = createFlags;
    imgCreateInfo.imageType = type;
    imgCreateInfo.tiling = tiling;
    imgCreateInfo.usage = imageUsage;
    imgCreateInfo.samples = (VkSampleCountFlagBits)sampleCounts;
    imgCreateInfo.mipLevels = numOfMips;
    imgCreateInfo.format = EngineToVulkanAPI::vulkanDataFormat(dataFormat);
    imgCreateInfo.arrayLayers = layerCount;
    imgCreateInfo.extent = { dimensions.x, dimensions.y, dimensions.z };
    imgCreateInfo.initialLayout = tiling == VK_IMAGE_TILING_LINEAR ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage nextImage = VulkanGraphicsHelper::createImage(graphicsInstance, imgCreateInfo, featuresRequired);
    setLayerCount(imgCreateInfo.arrayLayers);
    setNumOfMips(imgCreateInfo.mipLevels);

    if (nextImage)
    {
        image = nextImage;
        graphicsDebugger->markObject(this);
        fatalAssertf(
            VulkanGraphicsHelper::allocateImageResource(graphicsInstance, this, isStagingResource()), "Memory allocation failed for resource"
        );
    }
    else
    {
        LOG_ERROR("VulkanImageResource", "Failed creating image {}", getObjectName().getChar());
    }
}

void VulkanImageResource::release()
{
    if (image)
    {
        IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
        for (const std::pair<const ImageViewTypeAndInfo, VkImageView> &imageView : createdImageViews)
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
    const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        // TODO(Jeslas) : Check if layer count is necessary in this case
        return dimensions.x * dimensions.y * dimensions.z * layerCount * formatInfo->pixelDataSize;
    }
    return 0;
}

bool VulkanImageResource::isValid() { return image != nullptr; }

String VulkanImageResource::getObjectName() const { return getResourceName(); }

uint64 VulkanImageResource::getDispatchableHandle() const { return (uint64)image; }

uint64 VulkanImageResource::requiredSize() const { return getResourceSize(); }

bool VulkanImageResource::canAllocateMemory() const { return image && requiredSize() > 0; }

VkImageView VulkanImageResource::createImageView(const ImageViewInfo &viewInfo, VkImageViewType imgViewType)
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
    imageViewCreateInfo.format = EngineToVulkanAPI::vulkanDataFormat(dataFormat);
    imageViewCreateInfo.viewType = imgViewType;
    imageViewCreateInfo.subresourceRange = { viewAspects, viewInfo.viewSubresource.baseMip, viewInfo.viewSubresource.mipCount,
                                             viewInfo.viewSubresource.baseLayer, viewInfo.viewSubresource.layersCount };
    imageViewCreateInfo.components = {
        EngineToVulkanAPI::vulkanComponentSwizzle(viewInfo.componentMapping.r),
        EngineToVulkanAPI::vulkanComponentSwizzle(viewInfo.componentMapping.g),
        EngineToVulkanAPI::vulkanComponentSwizzle(viewInfo.componentMapping.b),
        EngineToVulkanAPI::vulkanComponentSwizzle(viewInfo.componentMapping.a),
    };

    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    VkImageView imgView = VulkanGraphicsHelper::createImageView(graphicsInstance, imageViewCreateInfo);
    VulkanGraphicsHelper::debugGraphics(graphicsInstance)
        ->markObject(uint64(imgView), getResourceName() + TCHAR("_View"), VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW);
    return imgView;
}

VkImageView VulkanImageResource::getImageView(const ImageViewInfo &viewInfo, int32 imageViewType /*= -1*/)
{
    if (!isValid())
    {
        return nullptr;
    }
    VkImageViewType imgViewType = viewType;
    // we are not validating the entire view info just view types
    if (imageViewType > 0 && imageViewType != viewType && viewType != VK_IMAGE_VIEW_TYPE_1D)
    {
        // if layer count is 1 then possibilities are Any D can be same D or lower D array, Same D is
        // already checked in outer if
        if (layerCount == 1)
        {
            debugAssert(
                viewType != VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            );
            debugAssert(viewType != VK_IMAGE_VIEW_TYPE_3D || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY);
        }
        // else cube can be cube array or 3D cannot be 2D array if levels are not 1
        else
        {
            debugAssert(!(imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY && viewType == VK_IMAGE_VIEW_TYPE_3D));
            debugAssert(
                viewType != VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
                || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
            );
        }
        imgViewType = VkImageViewType(imageViewType);
    }

    const ImageViewTypeAndInfo viewTypeAndInfo{ imgViewType, viewInfo };

    VkImageView imageView = nullptr;
    const auto &foundItr = createdImageViews.find(viewTypeAndInfo);
    if (foundItr == createdImageViews.cend())
    {
        imageView = createImageView(viewInfo, imgViewType);
        if (imageView != nullptr)
        {
            createdImageViews[viewTypeAndInfo] = imageView;
        }
    }
    else
    {
        imageView = foundItr->second;
    }
    return imageView;
}
