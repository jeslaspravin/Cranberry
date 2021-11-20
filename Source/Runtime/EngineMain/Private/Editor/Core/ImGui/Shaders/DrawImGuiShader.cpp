#include "Math/Vector2D.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/Resources/Pipelines.h"


struct UiTransform
{
    Vector2D scale;
    Vector2D translate;
};

BEGIN_BUFFER_DEFINITION(UiTransform)
ADD_BUFFER_TYPED_FIELD(scale)
ADD_BUFFER_TYPED_FIELD(translate)
END_BUFFER_DEFINITION();

#define DRAW_IMGUI "DrawImGui"

class DrawImGui : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DrawImGui, , UniqueUtilityShaderConfig, );
private:
    DrawImGui();

public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static UiTransformBufferParamInfo UI_TRANSFORM_INFO;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "uiTransform", &UI_TRANSFORM_INFO }
        };


        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(DrawImGui)

DrawImGui::DrawImGui()
    : BaseType(DRAW_IMGUI)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
CREATE_GRAPHICS_PIPELINE_REGISTRANT(IMGUI_PIPELINE_REGISTER, DRAW_IMGUI, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadOverBlendConfig);