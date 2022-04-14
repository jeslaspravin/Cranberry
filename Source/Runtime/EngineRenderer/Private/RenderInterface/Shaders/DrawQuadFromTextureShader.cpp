/*!
 * \file DrawQuadFromTextureShader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"

#define DRAW_QUAD_FROM_TEXTURE TCHAR("DrawQuadFromTexture")

class DrawQuadFromTexture : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DrawQuadFromTexture, , UniqueUtilityShaderConfig, );

private:
    DrawQuadFromTexture();
};

DEFINE_GRAPHICS_RESOURCE(DrawQuadFromTexture)

DrawQuadFromTexture::DrawQuadFromTexture()
    : BaseType(DRAW_QUAD_FROM_TEXTURE)
{}

#define DRAW_OVER_BLENDED_QUAD_FROM_TEXTURE TCHAR("DrawOverBlendedQuadFromTexture")
class DrawOverBlendedQuadFromTexture : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DrawOverBlendedQuadFromTexture, , UniqueUtilityShaderConfig, );

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
CREATE_GRAPHICS_PIPELINE_REGISTRANT(QUAD_FROM_TEXTURE_PIPELINE_REGISTER, DRAW_QUAD_FROM_TEXTURE,
    &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig);
CREATE_GRAPHICS_PIPELINE_REGISTRANT(OVER_BLENDED_QUAD_FROM_TEXTURE_PIPELINE_REGISTER,
    DRAW_OVER_BLENDED_QUAD_FROM_TEXTURE,
    &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadOverBlendConfig);