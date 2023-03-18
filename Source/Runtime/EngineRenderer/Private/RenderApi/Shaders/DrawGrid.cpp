/*!
 * \file DrawGrid.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/Scene/RenderScene.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define DRAW_GRID_NAME TCHAR("DrawGrid")

template <bool DepthTest>
GraphicsPipelineConfig drawGridPipelineConfig(String &pipelineName, const ShaderResource *shaderResource)
{
    pipelineName = shaderResource->getResourceName();
    GraphicsPipelineConfig config;
    if constexpr (DepthTest)
    {
        config = ScreenSpaceQuadPipelineConfigs::screenSpaceQuadOverBlendDepthTestedShaderConfig(pipelineName, shaderResource);
    }
    else
    {
        config = ScreenSpaceQuadPipelineConfigs::screenSpaceQuadOverBlendConfig(pipelineName, shaderResource);
    }

    // Since grid has to be visible either side
    config.supportedCullings.clear();
    config.supportedCullings.emplace_back(ECullingMode::None);

    return config;
}

template <bool DepthTest>
class DrawGrid : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DrawGrid, <EXPAND_ARGS(DepthTest)>, UniqueUtilityShaderConfig, );

private:
    String shaderFileName;

    DrawGrid()
        : BaseType(
            String(DRAW_GRID_NAME) + (DepthTest ? TCHAR("DTest") : TCHAR("")))
            , shaderFileName(DRAW_GRID_NAME)
    {
        static CREATE_GRAPHICS_PIPELINE_REGISTRANT(DRAW_GRID_PIPELINE_REGISTRAR, getResourceName().getChar(), &drawGridPipelineConfig<DepthTest>);
    }

protected:
    /* UniqueUtilityShader overrides */
    String getShaderFileName() const override { return shaderFileName; }
    /* overrides ends */
public:
    void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        static std::map<StringID, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{ RenderSceneBase::sceneViewParamInfo() };

        for (const std::pair<const StringID, ShaderBufferParamInfo *> &bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DrawGrid, <EXPAND_ARGS(bool DepthTest)>, <EXPAND_ARGS(DepthTest)>);

template DrawGrid<false>;
template DrawGrid<true>;