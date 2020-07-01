#include "DrawQuadFromInputAttachment.h"

DEFINE_GRAPHICS_RESOURCE(DrawQuadFromInputAttachment);

DrawQuadFromInputAttachment::DrawQuadFromInputAttachment() : BaseType("DrawQuadFromInputAttachment")
{
    compatibleVertex = EVertexType::Simple3;
    compatibleRenderpassFormat = ERenderpassFormat::Multibuffers;
}

