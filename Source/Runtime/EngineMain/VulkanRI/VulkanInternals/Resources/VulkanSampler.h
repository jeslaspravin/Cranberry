#pragma once
#include "../../../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"

#include <vulkan_core.h>

class VulkanSampler final : public SamplerInterface, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanSampler,,SamplerInterface,)
private:
    class VulkanDevice* ownerDevice;
public:
    VkSampler sampler;
private:
    VulkanSampler();
public:
    VulkanSampler(class VulkanDevice* device, ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering,
        float poorMipLod = 0, uint8 samplerBorderColFlags = 0);

    /* IVulkanResources Overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;

    /* GraphicsResources Overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    /* Overrides ends */
};