#include "RenderInterface/Shaders/EngineShaders/PBRShaders.h"
#include "Types/CoreDefines.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderApi/Scene/RenderScene.h"
#include "RenderInterface/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "Engine/Config/EngineGlobalConfigs.h"
#include "RenderInterface/Resources/Pipelines.h"

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

BEGIN_BUFFER_DEFINITION(ShadowData)
ADD_BUFFER_TYPED_FIELD(sptLitsW2C)
ADD_BUFFER_TYPED_FIELD(dirLitCascadesW2C)
ADD_BUFFER_TYPED_FIELD(cascadeFarPlane)
ADD_BUFFER_TYPED_FIELD(shadowFlags)
END_BUFFER_DEFINITION();

#define PBRLIGHTSNOSHADOW_SHADER_NAME "PBRLightsNoShadow"
#define PBRLIGHTSWITHSHADOW_SHADER_NAME "PBRLightsWithShadow"

class PBRShaders : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(PBRShaders, , UniqueUtilityShaderConfig, )
private:
    PBRShaders() = default;
protected:
    PBRShaders(const String& name)
        : BaseType(name)
    {}
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static PBRLightArrayBufferParamInfo LIGHTDATA_INFO;
        static ColorCorrectionBufferParamInfo COLOR_CORRECTION_INFO;
        static ShadowDataBufferParamInfo SHADOW_DATA_INFO;
        auto ShaderParamInfoInit = []
        {
            std::map<String, ShaderBufferParamInfo*> paramInfo
            {
                { "lightArray", &LIGHTDATA_INFO },
                { "colorCorrection", &COLOR_CORRECTION_INFO },
                { "shadowData", &SHADOW_DATA_INFO }
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

            if (foundDescBinding != bindingBuffers.end())
            {
                foundDescBinding->second->bufferParamInfo = bufferInfo.second;
            }
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(PBRShaders)

class PBRLightsNoShadowShader : public PBRShaders
{
    DECLARE_GRAPHICS_RESOURCE(PBRLightsNoShadowShader, , PBRShaders, )
protected:
    PBRLightsNoShadowShader()
        : BaseType(PBRLIGHTSNOSHADOW_SHADER_NAME)
    {}
public:
};

DEFINE_GRAPHICS_RESOURCE(PBRLightsNoShadowShader)

class PBRLightsWithShadowShader : public PBRShaders
{
    DECLARE_GRAPHICS_RESOURCE(PBRLightsWithShadowShader, , PBRShaders, )
protected:
    PBRLightsWithShadowShader()
        : BaseType(PBRLIGHTSWITHSHADOW_SHADER_NAME)
    {}
public:
    void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const override
    {
        specializationConst["PCF_KERNEL_SIZE"] = SpecializationConstUtility::fromValue(EngineSettings::pcfKernelSize.get());
        specializationConst["POINT_PCF_SAMPLES"] = SpecializationConstUtility::fromValue(EngineSettings::pointPcfKernelSize.get());
        specializationConst["POINT_PCF_KERNEL_EXTEND"] = SpecializationConstUtility::fromValue(0.2f);
    }
};

DEFINE_GRAPHICS_RESOURCE(PBRLightsWithShadowShader)

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

CREATE_GRAPHICS_PIPELINE_REGISTRANT(PBRNOSHADOW_SHADER_PIPELINE_REGISTER, PBRLIGHTSNOSHADOW_SHADER_NAME, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig);
CREATE_GRAPHICS_PIPELINE_REGISTRANT(PBRWITHSHADOW_SHADER_PIPELINE_REGISTER, PBRLIGHTSWITHSHADOW_SHADER_NAME, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig);
