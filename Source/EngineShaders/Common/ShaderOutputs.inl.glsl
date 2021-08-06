#ifndef SHADEROUTPUTS_INCLUDE
#define SHADEROUTPUTS_INCLUDE

#if MULTIBUFFER
layout(location = 0) out vec4 colorAttachment0;// Color
layout(location = 1) out vec4 colorAttachment1;// Normal
layout(location = 2) out vec4 colorAttachment2;// AO, Roughness, Metallic
#endif
#if DEPTH
#endif

#endif // SHADEROUTPUTS_INCLUDE