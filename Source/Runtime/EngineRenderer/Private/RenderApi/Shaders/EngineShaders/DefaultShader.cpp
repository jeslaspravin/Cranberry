/*!
 * \file DefaultShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/EngineShaders/ShadowDepthDraw.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformAssertionErrors.h"

template <EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class DefaultShader : public DrawMeshShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DefaultShader, <EXPAND_ARGS(VertexUsage, RenderpassFormat)>, DrawMeshShaderConfig, )
protected:
    DefaultShader()
        : BaseType(DEFAULT_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }

public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        if constexpr (RenderpassFormat != ERenderPassFormat::DirectionalLightDepth && RenderpassFormat != ERenderPassFormat::PointLightDepth)
        {
            return;
        }

        const std::map<String, ShaderBufferParamInfo *> *paramInfo = nullptr;
        if constexpr (RenderpassFormat == ERenderPassFormat::DirectionalLightDepth)
        {
            static const std::map<String, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{
                {TCHAR("lightViews"), DirectionalShadowCascadeViews::paramInfo()}
            };

            paramInfo = &SHADER_PARAMS_INFO;
        }
        else if constexpr (RenderpassFormat == ERenderPassFormat::PointLightDepth)
        {
            static const std::map<String, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{
                {TCHAR("lightViews"), PointShadowDepthViews::paramInfo()}
            };

            paramInfo = &SHADER_PARAMS_INFO;
        }

        if (paramInfo != nullptr)
        {
            for (const std::pair<const String, ShaderBufferParamInfo *> &bufferInfo : *paramInfo)
            {
                auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

                debugAssert(foundDescBinding != bindingBuffers.end());

                foundDescBinding->second->bufferParamInfo = bufferInfo.second;
            }
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DefaultShader, <EXPAND_ARGS(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>, <EXPAND_ARGS(VertexUsage, RenderpassFormat)>)

template class DefaultShader<EVertexType::Simple2, ERenderPassFormat::Multibuffer>;

template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffer>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::Depth>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::PointLightDepth>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::DirectionalLightDepth>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

CREATE_GRAPHICS_PIPELINE_REGISTRANT(
    DEFAULT_SHADER_PIPELINE_REGISTER, DEFAULT_SHADER_NAME, &CommonGraphicsPipelineConfigs::writeGbufferShaderConfig
);
