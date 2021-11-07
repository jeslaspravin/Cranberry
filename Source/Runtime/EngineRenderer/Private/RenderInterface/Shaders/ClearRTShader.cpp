#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"

#define CLEAR_RT "ClearRT"

struct ClearRTClearInfo
{
    Vector4D clearColor;
};

BEGIN_BUFFER_DEFINITION(ClearRTClearInfo)
ADD_BUFFER_TYPED_FIELD(clearColor)
END_BUFFER_DEFINITION();

class ClearRT : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(ClearRT, , UniqueUtilityShader, );
private:
    ClearRT();
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static ClearRTClearInfoBufferParamInfo CLEAR_INFO;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "clearInfo", &CLEAR_INFO }
        };


        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
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
ScreenSpaceQuadShaderPipelineRegistrar CLEAR_RT_PIPELINE_REGISTER(CLEAR_RT);