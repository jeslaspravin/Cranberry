
layout(set = 0, binding = 0) uniform ViewData
{
    mat4 view;
    mat4 invView;
    mat4 projection;
    mat4 invProjection;
} viewData;

// TODO(Jeslas) : Can be simplified for orthographics camera
vec3 getWorldPosition(vec4 screenPos)
{
    vec4 world = viewData.invProjection * screenPos;
    world = world/world.w;
    world = viewData.view * world;
    return world.xyz;
}

vec3 getViewSpacePosition(vec4 screenPos)
{
    vec4 viewSpace = viewData.invProjection * screenPos;
    viewSpace = viewSpace/viewSpace.w;
    return viewSpace.xyz;
}

vec3 viewFwd()
{
    return viewData.view[2].xyz;
}

vec3 viewPos()
{
    return viewData.view[3].xyz;
}