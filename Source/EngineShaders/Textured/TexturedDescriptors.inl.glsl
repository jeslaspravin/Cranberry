
layout( set = 2, binding = 0) uniform MeshData
{
    vec4 meshColor;
    vec4 rm_uvScale;
} meshData;

layout( set = 2, binding = 1) uniform sampler2D diffuseMap;
layout( set = 2, binding = 2) uniform sampler2D normalMap;
layout( set = 2, binding = 3) uniform sampler2D armMap; 