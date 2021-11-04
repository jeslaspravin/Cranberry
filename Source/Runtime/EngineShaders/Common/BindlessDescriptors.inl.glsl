#ifndef BINDLESSDESCRIPTORS_INCLUDE
#define BINDLESSDESCRIPTORS_INCLUDE

#include "CommonDefines.inl.glsl"

layout(set = BINDLESS_SET, binding = 0) uniform sampler2D globalSampledTexs[];
//layout(set = BINDLESS_SET, binding = 1) uniform texture2D globalTextures[];
//layout(set = BINDLESS_SET, binding = 2) uniform readonly image2D globalReadImages[];
//layout(set = BINDLESS_SET, binding = 3) uniform samplerBuffer globalReadTexels[];

//layout(set = BINDLESS_SET, binding = 4, rgba8ui) uniform image2D globalWriteImages[];
//layout(set = BINDLESS_SET, binding = 5, rgba8u) uniform imageBuffer globalReadTexels[];

#endif // BINDLESSDESCRIPTORS_INCLUDE