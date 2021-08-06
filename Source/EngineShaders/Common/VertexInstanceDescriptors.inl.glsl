#ifndef VERTEXINSTANCEDESCRIPTORS_INCLUDE
#define VERTEXINSTANCEDESCRIPTORS_INCLUDE

layout(set = 1, binding = 0) uniform InstanceData
{
    mat4 model;
    mat4 invModel;
} instanceData;

#endif // VERTEXINSTANCEDESCRIPTORS_INCLUDE