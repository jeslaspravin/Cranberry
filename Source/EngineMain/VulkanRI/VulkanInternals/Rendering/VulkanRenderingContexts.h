#pragma once
#include "../../../RenderInterface/Rendering/RenderingContexts.h"

class VulkanGlobalRenderingContext : public GlobalRenderingContextBase
{
private:
    std::unordered_map<GenericRenderpassProperties, VkRenderPass> genericRenderpasses;
    std::unordered_map<ERenderpassFormat::Type, VkRenderPass> gbufferRenderpasses;
protected:
	void initApiFactories() override;
};


namespace GraphicsTypes 
{
    typedef VulkanGlobalRenderingContext GlobalRenderingContext;
}