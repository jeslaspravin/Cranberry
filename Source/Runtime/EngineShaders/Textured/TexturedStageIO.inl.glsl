/*!
 * \file TexturedStageIO.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#ifndef TEXTUREDSTAGEIO_INCLUDE
#define TEXTUREDSTAGEIO_INCLUDE

#if INPUT
#define PREFIX in
#define VAR(name) in##name
#elif OUTPUT
#define PREFIX out
#define VAR(name) out##name
#endif

// Below macro replacement is not available in glslangvalidator
//#define VAR(name) PREFIX##name

#if STATIC_MESH
layout(location = 0) PREFIX vec3 VAR(WorldPosition);
layout(location = 1) PREFIX vec3 VAR(WorldNormal);
layout(location = 2) PREFIX vec3 VAR(WorldTangent);
layout(location = 3) PREFIX vec2 VAR(TextureCoord);
layout(location = 4) PREFIX flat uint VAR(MaterialIdx);
#endif

#undef PREFIX
#undef VAR

#endif // TEXTUREDSTAGEIO_INCLUDE