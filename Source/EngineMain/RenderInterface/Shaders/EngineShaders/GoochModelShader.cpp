#include "GoochModelShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"
#include "../../GlobalRenderVariables.h"
#include "../Base/UtilityShaders.h"
#include "../../../RenderApi/Scene/RenderScene.h"
#include "../Base/ScreenspaceQuadGraphicsPipeline.h"

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

#define GOOCH_SHADER_NAME "GoochModel"

class GoochModelShader : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(GoochModelShader, , UniqueUtilityShader, )
protected:
    GoochModelShader()
        : BaseType(GOOCH_SHADER_NAME)
    {}
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static GoochModelLightCommonBufferParamInfo LIGHTCOMMON_INFO;
        static GoochModelLightDataBufferParamInfo LIGHTDATA_INFO;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "lightCommon", &LIGHTCOMMON_INFO },
            { "light", &LIGHTDATA_INFO },
            { "viewData", RenderSceneBase::sceneViewParamInfo().at("viewData") }
        };


        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
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

ScreenSpaceQuadShaderPipelineRegister GOOCHMODEL_SHADER_PIPELINE_REGISTER(GOOCH_SHADER_NAME);