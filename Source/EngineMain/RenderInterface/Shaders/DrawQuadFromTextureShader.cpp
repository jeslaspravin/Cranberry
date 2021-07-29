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


#define DRAW_OVER_BLENDED_QUAD_FROM_TEXTURE "DrawOverBlendedQuadFromTexture"
class DrawOverBlendedQuadFromTexture : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(DrawOverBlendedQuadFromTexture, , UniqueUtilityShader, );
private:
    DrawOverBlendedQuadFromTexture();

protected:
    /* UniqueUtilityShader overrides */
    String getShaderFileName() const override { return DRAW_QUAD_FROM_TEXTURE; }
    /* overrides ends */
};

DEFINE_GRAPHICS_RESOURCE(DrawOverBlendedQuadFromTexture)

DrawOverBlendedQuadFromTexture::DrawOverBlendedQuadFromTexture()
    : BaseType(DRAW_OVER_BLENDED_QUAD_FROM_TEXTURE)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
ScreenSpaceQuadShaderPipelineRegistrar QUAD_FROM_TEXTURE_PIPELINE_REGISTER(DRAW_QUAD_FROM_TEXTURE);
OverBlendedSSQuadShaderPipelineRegistrar OVER_BLENDED_QUAD_FROM_TEXTURE_PIPELINE_REGISTER(DRAW_OVER_BLENDED_QUAD_FROM_TEXTURE);