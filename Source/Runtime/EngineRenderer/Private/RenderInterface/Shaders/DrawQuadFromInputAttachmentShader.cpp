/*!
 * \file DrawQuadFromInputAttachmentShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderInterface/Resources/Pipelines.h"

#define DRAW_QUAD_FROM_INPUT_ATTACHMENT "DrawQuadFromInputAttachment"

class DrawQuadFromInputAttachment : public ShaderConfigCollector/* UniqueUtilityShader Once support for input attachments are introduced */
{
    DECLARE_GRAPHICS_RESOURCE(DrawQuadFromInputAttachment, , ShaderConfigCollector, );
private:
    DrawQuadFromInputAttachment();
};

DEFINE_GRAPHICS_RESOURCE(DrawQuadFromInputAttachment);

DrawQuadFromInputAttachment::DrawQuadFromInputAttachment() 
    : BaseType(DRAW_QUAD_FROM_INPUT_ATTACHMENT)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
CREATE_GRAPHICS_PIPELINE_REGISTRANT(QUAD_FROM_INPUT_ATTACHMENT_PIPELINE_REGISTER, DRAW_QUAD_FROM_INPUT_ATTACHMENT, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig);
