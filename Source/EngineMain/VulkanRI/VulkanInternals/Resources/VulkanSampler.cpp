#include "VulkanSampler.h"
#include "../../VulkanGraphicsHelper.h"
#include "../VulkanDevice.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../VulkanGraphicsInstance.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanSampler, VK_OBJECT_TYPE_SAMPLER)

String VulkanSampler::getObjectName() const
{
    return getResourceName();
}

void VulkanSampler::setObjectName(const String& name)
{
    resourceName = name;
}

uint64 VulkanSampler::getDispatchableHandle() const
{
    return (uint64)sampler;
}

VulkanSampler::VulkanSampler() 
    : BaseType()
    , ownerDevice(nullptr)
    , sampler(nullptr)
{}

VulkanSampler::VulkanSampler(class VulkanDevice* device, ESamplerTilingMode::Type samplerTiling,
    ESamplerFiltering::Type samplerFiltering, float poorMipLod)
    : BaseType(samplerTiling,samplerFiltering, poorMipLod)
    , ownerDevice(device)
    , sampler(nullptr)
{}

void VulkanSampler::init()
{
    reinitResources();
}

void VulkanSampler::reinitResources()
{
    SAMPLER_CREATE_INFO(createInfo);
    createInfo.magFilter = (VkFilter)ESamplerFiltering::getFilterInfo(filtering)->filterTypeValue;
    createInfo.minFilter = VkFilter::VK_FILTER_NEAREST;
    createInfo.mipmapMode = (VkSamplerMipmapMode)ESamplerFiltering::getMipFilterInfo(mipFiltering)->filterTypeValue;
    createInfo.addressModeU = (VkSamplerAddressMode)ESamplerTilingMode::getSamplerTiling(tilingMode.x);
    createInfo.addressModeV = (VkSamplerAddressMode)ESamplerTilingMode::getSamplerTiling(tilingMode.y);
    createInfo.addressModeW = (VkSamplerAddressMode)ESamplerTilingMode::getSamplerTiling(tilingMode.z);
    // TODO(Jeslas) : following settings has to be obtained from global settings
    createInfo.mipLodBias = 0;
    createInfo.anisotropyEnable = true;
    createInfo.maxAnisotropy = 8;// PhysicalDeviceLimit check for this will be done at global level

    createInfo.compareEnable = useCompareOp;
    createInfo.compareOp = (VkCompareOp)CoreGraphicsTypes::getEnumTypeInfo(compareOp)->value;
    createInfo.minLod = mipLodRange.x;
    createInfo.maxLod = mipLodRange.y;

    uint32 borderCol = transparentBorder ? 0 : 2;
    borderCol += intBorder ? 1 : 0;
    borderCol += whiteBorder ? 2 : 0;
    createInfo.borderColor = (VkBorderColor)borderCol;

    VkSampler nextSampler;
    if (ownerDevice->vkCreateSampler(VulkanGraphicsHelper::getDevice(ownerDevice), &createInfo, nullptr, &nextSampler)
        == VK_SUCCESS)
    {
        release();
        sampler = nextSampler;
        ownerDevice->debugGraphics()->markObject(this);
    }
    else
    {
        Logger::error("VulkanSampler", "%s() : Initialization of sampler failed", __func__);
    }
}

void VulkanSampler::release()
{
    if (sampler)
    {
        ownerDevice->vkDestroySampler(VulkanGraphicsHelper::getDevice(ownerDevice), sampler, nullptr);
        sampler = nullptr;
    }
}
