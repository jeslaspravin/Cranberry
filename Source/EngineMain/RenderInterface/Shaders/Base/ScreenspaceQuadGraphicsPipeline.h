#pragma once

#include "../../PlatformIndependentHeaders.h"

class ScreenSpaceQuadShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(ScreenSpaceQuadShaderPipeline, , GraphicsPipeline, )
private:
    ScreenSpaceQuadShaderPipeline() = default;
public:
    ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent);
    ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource);
};

// Registrar
using ScreenSpaceQuadShaderPipelineRegister = GenericGraphicsPipelineRegister<ScreenSpaceQuadShaderPipeline>;