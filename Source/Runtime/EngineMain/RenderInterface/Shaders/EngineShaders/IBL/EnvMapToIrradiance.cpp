#include "../../Base/UtilityShaders.h"
#include "../../Base/GenericComputePipeline.h"
#include "../../../../Core/Engine/Config/EngineGlobalConfigs.h"

#define SAMPLE_COUNT "SAMPLE_COUNT"
#define ENVMAPTODIFFIRRAD_SHADER_NAME "EnvToDiffuseIrradiance"

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class EnvMapToDiffuseIrradiance final : public ComputeShaderTemplated<SizeX, SizeY, SizeZ>
{
    DECLARE_GRAPHICS_RESOURCE(EnvMapToDiffuseIrradiance, <ExpandArgs(SizeX, SizeY, SizeZ)>, ComputeShaderTemplated, <ExpandArgs(SizeX, SizeY, SizeZ)>)

public:
    EnvMapToDiffuseIrradiance()
        : BaseType(ENVMAPTODIFFIRRAD_SHADER_NAME)
    {
        static SimpleComputePipelineRegistrar ENVMAPTOIRRAD_SHADER_PIPELINE_REGISTER(this->getResourceName());
    }

    void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const final
    {
        specializationConst[SAMPLE_COUNT] = SpecializationConstUtility::fromValue(128u);
    }
};

template EnvMapToDiffuseIrradiance<4, 4, 1>;
template EnvMapToDiffuseIrradiance<16, 16, 1>;

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(EnvMapToDiffuseIrradiance, <ExpandArgs(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>, <ExpandArgs(SizeX, SizeY, SizeZ)>)

//////////////////////////////////////////////////////////////////////////
/// HDRI to Pre-filtered specular map
//////////////////////////////////////////////////////////////////////////

#define HDRITOPREFILTEREDSPEC_SHADER_NAME "HDRIToPrefilteredSpecMap"

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class HDRIToPrefilteredSpecular final : public ComputeShaderTemplated<SizeX, SizeY, SizeZ>
{
    DECLARE_GRAPHICS_RESOURCE(HDRIToPrefilteredSpecular, <ExpandArgs(SizeX, SizeY, SizeZ)>, ComputeShaderTemplated, <ExpandArgs(SizeX, SizeY, SizeZ)>)

public:
    HDRIToPrefilteredSpecular()
        : BaseType(HDRITOPREFILTEREDSPEC_SHADER_NAME)
    {
        static SimpleComputePipelineRegistrar HDRITOPREFILTEREDSPEC_SHADER_PIPELINE_REGISTER(this->getResourceName());
    }

    void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const final
    {
        specializationConst[SAMPLE_COUNT] = SpecializationConstUtility::fromValue(1024u);
        specializationConst["MIP_COUNT"] = SpecializationConstUtility::fromValue(EngineSettings::maxPrefilteredCubeMiplevels.get());
    }
};

template HDRIToPrefilteredSpecular<16, 16, 1>;

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(HDRIToPrefilteredSpecular, <ExpandArgs(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>, <ExpandArgs(SizeX, SizeY, SizeZ)>)