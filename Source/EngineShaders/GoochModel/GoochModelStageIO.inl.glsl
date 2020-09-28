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
layout(location = 0) PREFIX vec4 VAR(WorldPosition);
layout(location = 1) PREFIX vec3 VAR(WorldNormal);
layout(location = 2) PREFIX vec4 VAR(Color);
#endif

#undef PREFIX
#undef VAR
