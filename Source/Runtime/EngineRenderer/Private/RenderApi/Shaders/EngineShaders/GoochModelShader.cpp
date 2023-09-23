/*!
 * \file GoochModelShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/Shaders/EngineShaders/GoochModelShader.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Scene/RenderScene.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderApi/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformAssertionErrors.h"

BEGIN_BUFFER_DEFINITION(GoochModelLightCommon)
ADD_BUFFER_TYPED_FIELD(lightsCount)
ADD_BUFFER_TYPED_FIELD(invLightsCount)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(GoochModelLightData)
ADD_BUFFER_TYPED_FIELD(warmOffsetAndPosX)
ADD_BUFFER_TYPED_FIELD(coolOffsetAndPosY)
ADD_BUFFER_TYPED_FIELD(highlightColorAndPosZ)
ADD_BUFFER_TYPED_FIELD(lightColorAndRadius)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(GoochModelLightArray)
ADD_BUFFER_STRUCT_FIELD(lights, GoochModelLightData)
ADD_BUFFER_TYPED_FIELD(count)
END_BUFFER_DEFINITION();

#define GOOCH_SHADER_NAME TCHAR("GoochModel")

class GoochModelShader : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(GoochModelShader, , UniqueUtilityShaderConfig, )
protected:
    GoochModelShader()
        : BaseType(GOOCH_SHADER_NAME)
    {}

public:
    void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        static GoochModelLightCommonBufferParamInfo LIGHTCOMMON_INFO;
        static GoochModelLightArrayBufferParamInfo LIGHTDATA_INFO;
        static const std::map<StringID, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{
            {            TCHAR("lightCommon"),                                                                    &LIGHTCOMMON_INFO},
              {             TCHAR("lightArray"),                                                                      &LIGHTDATA_INFO},
                {RenderSceneBase::VIEW_PARAM_NAME, RenderSceneBase::sceneViewParamInfo().find(RenderSceneBase::VIEW_PARAM_NAME)->second}
        };

        for (const std::pair<const StringID, ShaderBufferParamInfo *> &bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(GoochModelShader)

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

CREATE_GRAPHICS_PIPELINE_REGISTRANT(
    GOOCHMODEL_SHADER_PIPELINE_REGISTER, GOOCH_SHADER_NAME, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig
);
