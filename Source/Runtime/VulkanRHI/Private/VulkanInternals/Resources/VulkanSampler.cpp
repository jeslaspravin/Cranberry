#include "VulkanInternals/Resources/VulkanSampler.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "Logger/Logger.h"
#include "VulkanGraphicsInstance.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "Math/Math.h"

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

VulkanSampler::VulkanSampler(class VulkanDevice* device, SamplerCreateInfo samplerCI)
    : BaseType(samplerCI)
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
    createInfo.minFilter = createInfo.magFilter = EngineToVulkanAPI::vulkanFilter(config.filtering);
    createInfo.mipmapMode = EngineToVulkanAPI::vulkanSamplerMipFilter(config.mipFiltering);
    createInfo.addressModeU = EngineToVulkanAPI::vulkanSamplerAddressing(std::get<0>(config.tilingMode));
    createInfo.addressModeV = EngineToVulkanAPI::vulkanSamplerAddressing(std::get<1>(config.tilingMode));
    createInfo.addressModeW = EngineToVulkanAPI::vulkanSamplerAddressing(std::get<2>(config.tilingMode));
    createInfo.mipLodBias = 0;
    createInfo.anisotropyEnable = GlobalRenderVariables::ENABLE_ANISOTROPY.get() && config.filtering != ESamplerFiltering::Cubic? VK_TRUE : VK_FALSE;
    // TODO(Jeslas) : Check if need to use some sort of asset type specific custom max limit, Instead of hardcoded 8
    createInfo.maxAnisotropy = Math::min(8.f, GlobalRenderVariables::MAX_ANISOTROPY.get());

    createInfo.compareEnable = config.useCompareOp;
    createInfo.compareOp = EngineToVulkanAPI::vulkanCompareOp(config.compareOp);
    createInfo.minLod = config.mipLodRange.minBound;
    createInfo.maxLod = config.mipLodRange.maxBound;

    uint32 borderCol = BIT_SET(config.borderColorFlags, ESamplerBorderColors::Transparent) ? 0 : 2;
    borderCol += BIT_SET(config.borderColorFlags, ESamplerBorderColors::Integer) ? 1 : 0;
    borderCol += BIT_SET(config.borderColorFlags, ESamplerBorderColors::White) ? 2 : 0;
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
