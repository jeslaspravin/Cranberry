#pragma once
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "VulkanInternals/Resources/IVulkanResources.h"
#include "VulkanInternals/VulkanMacros.h"

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
    VulkanSampler(class VulkanDevice* device, SamplerCreateInfo samplerCI);

    /* IVulkanResources Overrides */
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;

    /* GraphicsResources Overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    /* Overrides ends */
};