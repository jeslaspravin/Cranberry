/*!
 * \file ScreenspaceQuadGraphicsPipeline.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "EngineRendererExports.h"

struct GraphicsPipelineConfig;

namespace ScreenSpaceQuadPipelineConfigs
{
ENGINERENDERER_EXPORT GraphicsPipelineConfig screenSpaceQuadConfig(String &pipelineName, const ShaderResource *shaderResource);
ENGINERENDERER_EXPORT GraphicsPipelineConfig screenSpaceQuadOverBlendConfig(String &pipelineName, const ShaderResource *shaderResource);
ENGINERENDERER_EXPORT GraphicsPipelineConfig
screenSpaceQuadOverBlendDepthTestedShaderConfig(String &pipelineName, const ShaderResource *shaderResource);
} // namespace ScreenSpaceQuadPipelineConfigs