/*!
 * \file ShaderOutputs.inl.glsl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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