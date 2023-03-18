/*!
 * \file GoochModelStageIO.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#ifndef GOOCHMODELSTAGEIO_INCLUDE
#define GOOCHMODELSTAGEIO_INCLUDE

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
layout(location = 3) PREFIX vec2 VAR(PerspectiveZW);
#else
layout(location = 0) PREFIX vec2 VAR(TextureCoord);
layout(location = 1) PREFIX vec2 VAR(NdcCoord);
#endif

#undef PREFIX
#undef VAR

#endif // GOOCHMODELSTAGEIO_INCLUDE
