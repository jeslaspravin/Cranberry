#include "Base/UtilityShaders.h"
#include "Base/ScreenspaceQuadGraphicsPipeline.h"

#define DRAW_QUAD_FROM_TEXTURE "DrawQuadFromTexture"

class DrawQuadFromTexture : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(DrawQuadFromTexture, , UniqueUtilityShader, );
private:
    DrawQuadFromTexture();
};

DEFINE_GRAPHICS_RESOURCE(DrawQuadFromTexture)

DrawQuadFromTexture::DrawQuadFromTexture()
    : BaseType(DRAW_QUAD_FROM_TEXTURE)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
ScreenSpaceQuadShaderPipelineRegistrar QUAD_FROM_TEXTURE_PIPELINE_REGISTER(DRAW_QUAD_FROM_TEXTURE);