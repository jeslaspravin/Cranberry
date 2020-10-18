
layout(set = 0, binding = 0) uniform ViewData
{
    mat4 view;
    mat4 invView;
    mat4 projection;
    mat4 invProjection;
} viewData;

vec3 getWorldPosition(vec4 screenPos)
{
    vec4 world = viewData.invProjection * screenPos;
    world = world/world.w;
    world = viewData.view * world;
    return world.xyz;
}

vec3 viewFwd()
{
    return viewData.view[2].xyz;
}