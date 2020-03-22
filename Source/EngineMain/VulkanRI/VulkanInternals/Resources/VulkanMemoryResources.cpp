#include "VulkanMemoryResources.h"
#include "../../VulkanGraphicsHelper.h"
#include "../VulkanDevice.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"

#include <glm/exponential.hpp>
#include "glm/common.hpp"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanBufferResource, VK_OBJECT_TYPE_BUFFER)

void VulkanBufferResource::init()
{
    reinitResources();
}

void VulkanBufferResource::reinitResources()
{
    if (getResourceSize() == 0)
    {
        Logger::error("VulkanBufferResource", "%s() : Invalid resource %s", __func__, bufferName.getChar());
        return;
    }

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    const VulkanDebugGraphics* graphicsDebugger = VulkanGraphicsHelper::debugGraphics(graphicsInstance);

    BUFFER_CREATE_INFO(bufferCreateInfo);
    bufferCreateInfo.size = requiredSize();
    bufferCreateInfo.usage = bufferUsage;

    VkBuffer nextBuffer = VulkanGraphicsHelper::createBuffer(graphicsInstance, &bufferCreateInfo, dataFormat);

    if (nextBuffer)
    {
        release();
        buffer = nextBuffer;
        graphicsDebugger->markObject(this);
        VulkanGraphicsHelper::allocateBufferResource(graphicsInstance, this, isStagingResource());
    }
    else
    {
        Logger::error("VulkanBufferResource", "%s() : Failed creating buffer %s", __func__, bufferName.getChar());
    }
}

void VulkanBufferResource::release()
{
    if (buffer)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
        VulkanGraphicsHelper::deallocateBufferResource(graphicsInstance, this);
        VulkanGraphicsHelper::destroyBuffer(graphicsInstance, buffer);
        buffer = nullptr;
    }
}

String VulkanBufferResource::getObjectName() const
{
    return getResourceName();
}

void VulkanBufferResource::setObjectName(const String& name)
{
    bufferName = name;
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

String VulkanBufferResource::getResourceName() const
{
    return bufferName;
}

bool VulkanBufferResource::isValid()
{
    return buffer != nullptr;
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
{
    if (cpuAccessible)
    {
        tiling = VK_IMAGE_TILING_LINEAR;
        bIsStagingResource = true;
    }
}

VulkanImageResource::VulkanImageResource()
    : ImageResource(EPixelDataFormat::RGBA_U8_Packed)
    , defaultImageUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    , defaultFeaturesRequired(VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
    , createFlags(0)
    , tiling(VK_IMAGE_TILING_OPTIMAL)
    , type(VK_IMAGE_TYPE_2D)
{}

void VulkanImageResource::init()
{
    reinitResources();
}

void VulkanImageResource::reinitResources()
{
    VkImageUsageFlags imageUsage = defaultImageUsage;
    VkFormatFeatureFlags featuresRequired = defaultFeaturesRequired;
    if (isRenderTarget)
    {
        imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        featuresRequired = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        const EPixelDataFormat::ImageFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

        if (!formatInfo)
        {
            Logger::error("VulkanImageResource", "%s() : Not supported image format", __func__);
            return;
        }
        if (formatInfo->format == VK_FORMAT_D16_UNORM || formatInfo->format == VK_FORMAT_X8_D24_UNORM_PACK32
            || formatInfo->format == VK_FORMAT_D16_UNORM_S8_UINT || formatInfo->format == VK_FORMAT_D24_UNORM_S8_UINT
            || formatInfo->format == VK_FORMAT_D32_SFLOAT || formatInfo->format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            featuresRequired |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else
        {
            imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            featuresRequired |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
        }
        tiling = VK_IMAGE_TILING_OPTIMAL;
        numOfMips = 1;
    }
    else
    {
        if (numOfMips == 0)
        {
            // TODO (Jeslas) : Check if 1D or 3D can have more mips
            numOfMips = (uint32)(1 + glm::floor(glm::log2((float)glm::max(dimensions.x, dimensions.y))));
        }
        if(type != VK_IMAGE_TYPE_2D)
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
        featuresRequired |= ((shaderUsage & EImageShaderUsage::Writing) > 0) ? VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
            | VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT : 0;

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
        Logger::error("VulkanImageResource", "%s() : Invalid resource %s", __func__, imageName.getChar());
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
    imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage nextImage = VulkanGraphicsHelper::createImage(graphicsInstance, imgCreateInfo, featuresRequired);
    setLayerCount(imgCreateInfo.arrayLayers);
    setNumOfMips(imgCreateInfo.mipLevels);

    if (nextImage)
    {
        release();
        image = nextImage;
        graphicsDebugger->markObject(this);
        VulkanGraphicsHelper::allocateImageResource(graphicsInstance, this, isStagingResource());
    }
    else
    {
        Logger::error("VulkanImageResource", "%s() : Failed creating image %s", __func__, imageName.getChar());
    }
}

void VulkanImageResource::release()
{
    if (image)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
        VulkanGraphicsHelper::deallocateImageResource(graphicsInstance, this);
        VulkanGraphicsHelper::destroyImage(graphicsInstance, image);
        image = nullptr;
    }
}

String VulkanImageResource::getResourceName() const
{
    return imageName;
}

uint64 VulkanImageResource::getResourceSize() const
{
    const EPixelDataFormat::ImageFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

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

void VulkanImageResource::setObjectName(const String& name)
{
    imageName = name;
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