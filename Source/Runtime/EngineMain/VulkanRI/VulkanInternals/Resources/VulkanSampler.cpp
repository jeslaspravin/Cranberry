#include "VulkanSampler.h"
#include "../../VulkanGraphicsHelper.h"
#include "../VulkanDevice.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../VulkanGraphicsInstance.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"
#include "../../../Core/Math/Math.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanSampler, VK_OBJECT_TYPE_SAMPLER)

String VulkanSampler::getObjectName() const
{
    return getResourceName();
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
    ESamplerFiltering::Type samplerFiltering, float poorMipLod /*= 0*/, uint8 samplerBorderColFlags /*= 0*/)
    : BaseType(samplerTiling,samplerFiltering, poorMipLod, samplerBorderColFlags)
    , ownerDevice(device)
    , sampler(nullptr)
{}

void VulkanSampler::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanSampler::reinitResources()
{
    release();
    BaseType::reinitResources();
    SAMPLER_CREATE_INFO(createInfo);
    createInfo.magFilter = (VkFilter)ESamplerFiltering::getFilterInfo(filtering)->filterTypeValue;
    createInfo.minFilter = (VkFilter)ESamplerFiltering::getFilterInfo(filtering)->filterTypeValue;
    createInfo.mipmapMode = (VkSamplerMipmapMode)ESamplerFiltering::getMipFilterInfo(mipFiltering)->filterTypeValue;
    createInfo.addressModeU = (VkSamplerAddressMode)ESamplerTilingMode::getSamplerTiling(tilingMode.x);
    createInfo.addressModeV = (VkSamplerAddressMode)ESamplerTilingMode::getSamplerTiling(tilingMode.y);
    createInfo.addressModeW = (VkSamplerAddressMode)ESamplerTilingMode::getSamplerTiling(tilingMode.z);
    createInfo.mipLodBias = 0;
    createInfo.anisotropyEnable = GlobalRenderVariables::ENABLE_ANISOTROPY.get() && filtering != ESamplerFiltering::Cubic? VK_TRUE : VK_FALSE;
    // TODO(Jeslas) : Check if need to use some sort of asset type specific custom max limit, Instead of hardcoded 8
    createInfo.maxAnisotropy = Math::min(8.f,GlobalRenderVariables::MAX_ANISOTROPY.get());

    createInfo.compareEnable = useCompareOp;
    createInfo.compareOp = (VkCompareOp)CoreGraphicsTypes::getEnumTypeInfo(compareOp)->value;
    createInfo.minLod = mipLodRange.x;
    createInfo.maxLod = mipLodRange.y;

    uint32 borderCol = BIT_SET(borderColorFlags, ESamplerBorderColors::Transparent) ? 0 : 2;
    borderCol += BIT_SET(borderColorFlags, ESamplerBorderColors::Integer) ? 1 : 0;
    borderCol += BIT_SET(borderColorFlags, ESamplerBorderColors::White) ? 2 : 0;
    createInfo.borderColor = (VkBorderColor)borderCol;

    VkSampler nextSampler;
    if (ownerDevice->vkCreateSampler(VulkanGraphicsHelper::getDevice(ownerDevice), &createInfo, nullptr, &nextSampler)
        == VK_SUCCESS)
    {
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
    BaseType::release();
}
