#include "PBRShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"
#include "../../GlobalRenderVariables.h"
#include "../Base/UtilityShaders.h"
#include "../../../RenderApi/Scene/RenderScene.h"
#include "../Base/ScreenspaceQuadGraphicsPipeline.h"

BEGIN_BUFFER_DEFINITION(PbrSpotLight)
ADD_BUFFER_TYPED_FIELD(sptLightColor_lumen)
ADD_BUFFER_TYPED_FIELD(sptPos_radius)
ADD_BUFFER_TYPED_FIELD(sptDirection)
ADD_BUFFER_TYPED_FIELD(sptCone)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(PbrPointLight)
ADD_BUFFER_TYPED_FIELD(ptLightColor_lumen)
ADD_BUFFER_TYPED_FIELD(ptPos_radius)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(PbrDirectionalLight)
ADD_BUFFER_TYPED_FIELD(lightColor_lumen)
ADD_BUFFER_TYPED_FIELD(direction)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(PBRLightArray)
ADD_BUFFER_TYPED_FIELD(count)
ADD_BUFFER_STRUCT_FIELD(spotLits, PbrSpotLight)
ADD_BUFFER_STRUCT_FIELD(ptLits, PbrPointLight)
ADD_BUFFER_STRUCT_FIELD(dirLit, PbrDirectionalLight)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(ColorCorrection)
ADD_BUFFER_TYPED_FIELD(exposure)
ADD_BUFFER_TYPED_FIELD(gamma)
END_BUFFER_DEFINITION();

#define PBR_SHADER_NAME "PBR"

class PBRShader : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(PBRShader, , UniqueUtilityShader, )
protected:
    PBRShader()
        : BaseType(PBR_SHADER_NAME)
    {}
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static PBRLightArrayBufferParamInfo LIGHTDATA_INFO;
        static ColorCorrectionBufferParamInfo COLOR_CORRECTION;
        auto ShaderParamInfoInit = []
        {
            std::map<String, ShaderBufferParamInfo*> paramInfo
            {
                { "lightArray", &LIGHTDATA_INFO },
                { "colorCorrection", &COLOR_CORRECTION}
            };
            paramInfo.insert(RenderSceneBase::sceneViewParamInfo().cbegin(), RenderSceneBase::sceneViewParamInfo().cend());
            return paramInfo;
        };
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            ShaderParamInfoInit()
        };

        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(PBRShader)

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

ScreenSpaceQuadShaderPipelineRegistrar PBR_SHADER_PIPELINE_REGISTER(PBR_SHADER_NAME);
