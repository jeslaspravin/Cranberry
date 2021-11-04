#version 450
#extension GL_GOOGLE_include_directive:enable

#define SIMPLE3D 1
#include "../../Common/VertexInputs.inl.glsl"
#undef SIMPLE3D

#include "../../Common/ViewDescriptors.inl.glsl"

layout(push_constant) uniform Constants
{
    float gridExtendSize;
    float gridCellSize;
    // Minimum pixels a cell can cover before falling to lower LOD
    float cellMinPixelCoverage;
} constants;

layout(location = 0) out vec2 outGrid;
layout(location = 1) out vec3 outViewDir;
// flat = no interpolation
layout(location = 2) flat out vec2 outViewPos;

void mainVS()
{
//    vec3 gridCenter = ceil(viewPos() / constants.gridCellSize) * constants.gridCellSize;
//    outGrid = gridCenter.xy + (position * constants.gridExtendSize).xy;
    outViewPos = viewPos().xy;
    outGrid = outViewPos + (position * constants.gridExtendSize).xy;
    outViewDir = vec3(outGrid, 0) - viewPos();
    gl_Position = viewData.projection * viewData.invView * vec4(outGrid, 0, 1);
}