
#if MULTIBUFFER
layout(location = 0) out vec4 colorAttachment0;// Color
layout(location = 1) out vec4 colorAttachment1;// Normal
layout(location = 2) out float colorAttachment2;// Depth
#endif
#if DEPTH
layout(location = 0) out vec4 colorAttachment0;// Depth
#endif