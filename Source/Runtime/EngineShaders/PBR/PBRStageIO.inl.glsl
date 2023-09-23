/*!
 * \file PBRStageIO.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#ifndef PBRSTAGEIO_INCLUDE
#define PBRSTAGEIO_INCLUDE

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
layout(location = 2) PREFIX vec3 VAR(ViewFwd);
#else
layout(location = 0) PREFIX vec2 VAR(TextureCoord);
layout(location = 1) PREFIX vec2 VAR(NdcCoord);
#endif

#undef PREFIX
#undef VAR

#endif // PBRSTAGEIO_INCLUDE
