#pragma once
#include "../../../RenderInterface/Rendering/RenderingContexts.h"
#include "../../../RenderInterface/Rendering/FramebufferTypes.h"

#include <vulkan_core.h>

class DrawMeshShader;

class VulkanGlobalRenderingContext final : public GlobalRenderingContextBase
{
private:
    std::unordered_map<GenericRenderpassProperties, VkRenderPass> genericRenderpasses;
    std::unordered_map<ERenderpassFormat::Type, VkRenderPass> gbufferRenderpasses;

    std::unordered_map<const ShaderResource*, VkPipelineLayout> pipelineLayouts;
private:
    VkRenderPass createGbufferRenderpass(const DrawMeshShader* shaderResource) const;
protected:
    /* GlobalRenderingContextBase overrides */
	void initApiFactories() final;
    void initializeApiContext() final;
    void clearApiContext() final;
    void initializeNewPipeline(UniqueUtilityShaderObject* shaderObject, PipelineBase* pipeline) final;

    /* Override ends */
};


namespace GraphicsTypes 
{
    typedef VulkanGlobalRenderingContext GlobalRenderingContext;
}