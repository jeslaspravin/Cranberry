#pragma once
#include "EngineRendererExports.h"

struct GraphicsPipelineConfig;

namespace ScreenSpaceQuadPipelineConfigs
{
    ENGINERENDERER_EXPORT GraphicsPipelineConfig screenSpaceQuadConfig(String& pipelineName, const ShaderResource* shaderResource);
    ENGINERENDERER_EXPORT GraphicsPipelineConfig screenSpaceQuadOverBlendConfig(String& pipelineName, const ShaderResource* shaderResource);
    ENGINERENDERER_EXPORT GraphicsPipelineConfig screenSpaceQuadOverBlendDepthTestedShaderConfig(String& pipelineName, const ShaderResource* shaderResource);
}