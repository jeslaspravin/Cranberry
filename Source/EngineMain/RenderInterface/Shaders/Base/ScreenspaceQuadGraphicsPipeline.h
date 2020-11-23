#pragma once

#include "../../PlatformIndependentHeaders.h"

class ScreenSpaceQuadShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(ScreenSpaceQuadShaderPipeline, , GraphicsPipeline, )
protected:
    ScreenSpaceQuadShaderPipeline() = default;
public:
    ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent);
    ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource);
};

// Registrar
using ScreenSpaceQuadShaderPipelineRegister = GenericGraphicsPipelineRegister<ScreenSpaceQuadShaderPipeline>;

// Combine this with ScreenSpaceQuadShaderPipeline once blend states become dynamic permuted pipeline state
class OverBlendedSSQuadShaderPipeline : public ScreenSpaceQuadShaderPipeline
{
    DECLARE_GRAPHICS_RESOURCE(OverBlendedSSQuadShaderPipeline, , ScreenSpaceQuadShaderPipeline, )
private:
    OverBlendedSSQuadShaderPipeline() = default;
public:
    OverBlendedSSQuadShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent)
        : BaseType(shaderResource, parent)
    {}
    OverBlendedSSQuadShaderPipeline(const ShaderResource* shaderResource);
};

// Registrar
using OverBlendedSSQuadShaderPipelineRegister = GenericGraphicsPipelineRegister<OverBlendedSSQuadShaderPipeline>;