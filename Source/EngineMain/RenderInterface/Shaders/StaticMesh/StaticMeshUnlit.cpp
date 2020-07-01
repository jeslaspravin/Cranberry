#include "StaticMeshUnlit.h"

DEFINE_GRAPHICS_RESOURCE(StaticMeshUnlit)

StaticMeshUnlit::StaticMeshUnlit()
    : BaseType("StaticMeshUnlit")
{
    compatibleVertex = EVertexType::StaticMesh;
    compatibleRenderpassFormat = ERenderpassFormat::Multibuffers;
}
