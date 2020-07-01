#include "DrawQuadFromTexture.h"

DEFINE_GRAPHICS_RESOURCE(DrawQuadFromTexture)

DrawQuadFromTexture::DrawQuadFromTexture()
    : BaseType("DrawQuadFromTexture")
{
    compatibleVertex = EVertexType::Simple3;
    compatibleRenderpassFormat = ERenderpassFormat::Multibuffers;
}
