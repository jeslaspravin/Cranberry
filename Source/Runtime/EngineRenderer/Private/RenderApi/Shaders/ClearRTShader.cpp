/*!
 * \file ClearRTShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define CLEAR_RT TCHAR("ClearRT")

struct ClearRTClearInfo
{
    Vector4D clearColor;
};

BEGIN_BUFFER_DEFINITION(ClearRTClearInfo)
ADD_BUFFER_TYPED_FIELD(clearColor)
END_BUFFER_DEFINITION();

class ClearRT : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(ClearRT, , UniqueUtilityShaderConfig, );

private:
    ClearRT();

public:
    void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        static ClearRTClearInfoBufferParamInfo CLEAR_INFO;
        static const std::map<StringID, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{
            {TCHAR("clearInfo"), &CLEAR_INFO}
        };

        for (const std::pair<const StringID, ShaderBufferParamInfo *> &bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(ClearRT)

ClearRT::ClearRT()
    : BaseType(CLEAR_RT)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar

CREATE_GRAPHICS_PIPELINE_REGISTRANT(CLEAR_RT_PIPELINE_REGISTER, CLEAR_RT, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig);