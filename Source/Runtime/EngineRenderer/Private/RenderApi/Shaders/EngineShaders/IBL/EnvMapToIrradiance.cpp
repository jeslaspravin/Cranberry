/*!
 * \file EnvMapToIrradiance.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderApi/Rendering/PipelineRegistration.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "ShaderDataTypes.h"

#define SAMPLE_COUNT STRID("SAMPLE_COUNT")
#define ENVMAPTODIFFIRRAD_SHADER_NAME TCHAR("EnvToDiffuseIrradiance")

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class EnvMapToDiffuseIrradiance final : public ComputeShaderConfigTemplated<SizeX, SizeY, SizeZ>
{
    DECLARE_GRAPHICS_RESOURCE(EnvMapToDiffuseIrradiance, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>, ComputeShaderConfigTemplated, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>)

public:
    EnvMapToDiffuseIrradiance()
        : BaseType(ENVMAPTODIFFIRRAD_SHADER_NAME)
    {
        static ComputePipelineFactoryRegistrant ENVMAPTOIRRAD_SHADER_PIPELINE_REGISTER(this->getResourceName().getChar());
    }

    void getSpecializationConsts(SpecConstantNamedMap &specializationConst) const
    {
        specializationConst[SAMPLE_COUNT] = SpecializationConstUtility::fromValue(128u);
    }
};

template EnvMapToDiffuseIrradiance<4, 4, 1>;
template EnvMapToDiffuseIrradiance<16, 16, 1>;

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(EnvMapToDiffuseIrradiance, <EXPAND_ARGS(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>)

//////////////////////////////////////////////////////////////////////////
/// HDRI to Pre-filtered specular map
//////////////////////////////////////////////////////////////////////////

#define HDRITOPREFILTEREDSPEC_SHADER_NAME TCHAR("HDRIToPrefilteredSpecMap")

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class HDRIToPrefilteredSpecular final : public ComputeShaderConfigTemplated<SizeX, SizeY, SizeZ>
{
    DECLARE_GRAPHICS_RESOURCE(HDRIToPrefilteredSpecular, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>, ComputeShaderConfigTemplated, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>)

public:
    HDRIToPrefilteredSpecular()
        : BaseType(HDRITOPREFILTEREDSPEC_SHADER_NAME)
    {
        static ComputePipelineFactoryRegistrant HDRITOPREFILTEREDSPEC_SHADER_PIPELINE_REGISTER(this->getResourceName().getChar());
    }

    void getSpecializationConsts(SpecConstantNamedMap &specializationConst) const
    {
        specializationConst[SAMPLE_COUNT] = SpecializationConstUtility::fromValue(1024u);
        specializationConst[STRID("MIP_COUNT")] = SpecializationConstUtility::fromValue(GlobalRenderVariables::MAX_PREFILTERED_CUBE_MIPS.get());
    }
};

template HDRIToPrefilteredSpecular<16, 16, 1>;

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(HDRIToPrefilteredSpecular, <EXPAND_ARGS(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>, <EXPAND_ARGS(SizeX, SizeY, SizeZ)>)