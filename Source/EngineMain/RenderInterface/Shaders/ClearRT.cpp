#include "Base/UtilityShaders.h"
#include "Base/ScreenspaceQuadGraphicsPipeline.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../ShaderCore/ShaderParameters.h"
#include "../ShaderCore/ShaderParameterResources.h"

#define CLEAR_RT "ClearRT"

struct ClearRT_ClearInfo
{
    Vector4D clearColor;
};

BEGIN_BUFFER_DEFINITION(ClearRT_ClearInfo)
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
        static ClearRT_ClearInfoBufferParamInfo CLEAR_INFO;
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
ScreenSpaceQuadShaderPipelineRegister CLEAR_RT_PIPELINE_REGISTER(CLEAR_RT);