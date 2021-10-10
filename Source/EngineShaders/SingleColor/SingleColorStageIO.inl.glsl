#ifndef SINGLECOLORSTAGEIO_INCLUDE
#define SINGLECOLORSTAGEIO_INCLUDE

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
layout(location = 2) PREFIX flat uint VAR(MaterialIdx);
#elif SIMPLE2D
layout(location = 0) PREFIX vec3 VAR(WorldPosition);
layout(location = 1) PREFIX flat uint VAR(MaterialIdx);

#endif

#undef PREFIX
#undef VAR

#endif // SINGLECOLORSTAGEIO_INCLUDE
