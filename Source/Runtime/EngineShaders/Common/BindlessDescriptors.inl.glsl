/*!
 * \file BindlessDescriptors.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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