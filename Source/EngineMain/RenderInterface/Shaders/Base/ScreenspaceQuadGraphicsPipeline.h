#pragma once

#include "../../PlatformIndependentHeaders.h"

class ScreenSpaceQuadShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(ScreenSpaceQuadShaderPipeline, , GraphicsPipeline, )
protected:
    ScreenSpaceQuadShaderPipeline() = default;
public:
    ScreenSpaceQuadShaderPipeline(const PipelineBase* parent);
    ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource);
};

// Registrar
using ScreenSpaceQuadShaderPipelineRegistrar = GenericPipelineRegistrar<ScreenSpaceQuadShaderPipeline>;

// Combine this with ScreenSpaceQuadShaderPipeline once blend states become dynamic permuted pipeline state
class OverBlendedSSQuadShaderPipeline : public ScreenSpaceQuadShaderPipeline
{
    DECLARE_GRAPHICS_RESOURCE(OverBlendedSSQuadShaderPipeline, , ScreenSpaceQuadShaderPipeline, )
private:
    OverBlendedSSQuadShaderPipeline() = default;
public:
    OverBlendedSSQuadShaderPipeline(const PipelineBase* parent)
        : BaseType(parent)
    {}
    OverBlendedSSQuadShaderPipeline(const ShaderResource* shaderResource);
};

// Registrar
using OverBlendedSSQuadShaderPipelineRegistrar = GenericPipelineRegistrar<OverBlendedSSQuadShaderPipeline>;