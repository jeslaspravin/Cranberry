/*!
 * \file DebugVisDepthTextureShader.cpp
 *
 * \author Jeslas Pravin
 * \date September 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "RenderApi/Scene/RenderScene.h"

#define DEBUG_VIS_DEPTH_TEXTURE TCHAR("DebugVisDepthTexture")

class DebugVisDepthTexture : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DebugVisDepthTexture, , UniqueUtilityShaderConfig, );

private:
    DebugVisDepthTexture();

public:
    void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        for (const std::pair<const StringID, ShaderBufferParamInfo *> &bufferInfo : RenderSceneBase::sceneViewParamInfo())
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            if (foundDescBinding != bindingBuffers.end())
            {
                foundDescBinding->second->bufferParamInfo = bufferInfo.second;
            }
        }
    }

    void getSpecializationConsts(SpecConstantNamedMap &specializationConst) const override
    {
        specializationConst[STRID("DEPTH_NORMALIZE_RANGE")] = SpecializationConstUtility::fromValue(5000.f);
    }
};

DEFINE_GRAPHICS_RESOURCE(DebugVisDepthTexture)

DebugVisDepthTexture::DebugVisDepthTexture()
    : BaseType(DEBUG_VIS_DEPTH_TEXTURE)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
CREATE_GRAPHICS_PIPELINE_REGISTRANT(
    DEBUG_VIS_DEPTH_TEXTURE_PIPELINE_REGISTER, DEBUG_VIS_DEPTH_TEXTURE, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig
);