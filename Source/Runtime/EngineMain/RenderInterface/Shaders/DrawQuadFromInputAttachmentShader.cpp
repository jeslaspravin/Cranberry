#include "Base/UtilityShaders.h"
#include "Base/ScreenspaceQuadGraphicsPipeline.h"

#define DRAW_QUAD_FROM_INPUT_ATTACHMENT "DrawQuadFromInputAttachment"

class DrawQuadFromInputAttachment : public GraphicsShaderResource/* UniqueUtilityShader Once support for input attachments are introduced */
{
    DECLARE_GRAPHICS_RESOURCE(DrawQuadFromInputAttachment, , GraphicsShaderResource, );
private:
    DrawQuadFromInputAttachment();
};

DEFINE_GRAPHICS_RESOURCE(DrawQuadFromInputAttachment);

DrawQuadFromInputAttachment::DrawQuadFromInputAttachment() : BaseType(DRAW_QUAD_FROM_INPUT_ATTACHMENT)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
ScreenSpaceQuadShaderPipelineRegistrar QUAD_FROM_INPUT_ATTACHMENT_PIPELINE_REGISTER(DRAW_QUAD_FROM_INPUT_ATTACHMENT);
