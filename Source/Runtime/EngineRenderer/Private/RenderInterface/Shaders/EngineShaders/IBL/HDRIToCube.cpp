/*!
 * \file HDRIToCube.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Resources/Pipelines.h"

#define HDRITOCUBE_SHADER_NAME TCHAR("HDRIToCube")

class HDRIToCubeShader : public ComputeShaderConfigTemplated<16, 16, 1>
{
    DECLARE_GRAPHICS_RESOURCE(HDRIToCubeShader,, ComputeShaderConfigTemplated, <EXPAND_ARGS(16, 16, 1)>)

public:
    HDRIToCubeShader()
        : BaseType(HDRITOCUBE_SHADER_NAME)
    { 
        static ComputePipelineFactoryRegistrant HDRITOCUBE_SHADER_PIPELINE_REGISTER(getResourceName());
    }
};

DEFINE_GRAPHICS_RESOURCE(HDRIToCubeShader)

