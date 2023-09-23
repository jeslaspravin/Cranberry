/*!
 * \file HDRIToCube.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/Pipelines.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"

#define HDRITOCUBE_SHADER_NAME TCHAR("HDRIToCube")

class HDRIToCubeShader : public ComputeShaderConfigTemplated<16, 16, 1>
{
    DECLARE_GRAPHICS_RESOURCE(HDRIToCubeShader, , ComputeShaderConfigTemplated, <EXPAND_ARGS(16, 16, 1)>)

public:
    HDRIToCubeShader()
        : BaseType(HDRITOCUBE_SHADER_NAME)
    {
        static ComputePipelineFactoryRegistrant HDRITOCUBE_SHADER_PIPELINE_REGISTER(getResourceName().getChar());
    }
};

DEFINE_GRAPHICS_RESOURCE(HDRIToCubeShader)
