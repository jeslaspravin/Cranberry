#ifndef VERTEXINSTANCEDESCRIPTORS_INCLUDE
#define VERTEXINSTANCEDESCRIPTORS_INCLUDE

#include "CommonDefines.inl.glsl"

struct InstanceData
{
    mat4 model;
    mat4 invModel;
    // Index to shader unique param index
    uint shaderUniqIdx;
};

layout(set = INSTANCE_UNIQ_SET, binding = 0) readonly buffer Instances
{
    InstanceData instances[];
} instancesWrapper;

#endif // VERTEXINSTANCEDESCRIPTORS_INCLUDE