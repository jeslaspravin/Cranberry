
#if SIMPLE2D
layout(location = 0) in vec2 position;
#endif
#if UI
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
#endif
#if SIMPLE3D
layout(location = 0) in vec3 position;
#endif
#if SIMPLE3D_COLOR
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
#endif
#if INSTANCED_SIMPLE3D_COLOR
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 x;
layout(location = 3) in vec3 y;
layout(location = 4) in vec3 translation;
#endif
#if BASIC_MESH
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 textureCoord;
#endif
#if STATIC_MESH
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec4 tangent;
#endif