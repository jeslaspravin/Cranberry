#include "../../Base/UtilityShaders.h"
#include "../../Base/GenericComputePipeline.h"

#define ENVMAPTOIRRAD_SHADER_NAME "EnvToIrradiance"

template <uint32 SizeX, uint32 SizeY, uint32 SizeZ>
class EnvMapToIrradiance : public ComputeShaderTemplated<SizeX, SizeY, SizeZ>
{
    DECLARE_GRAPHICS_RESOURCE(EnvMapToIrradiance, <ExpandArgs(SizeX, SizeY, SizeZ)>, ComputeShaderTemplated, <ExpandArgs(SizeX, SizeY, SizeZ)>)

public:
    EnvMapToIrradiance()
        : BaseType(ENVMAPTOIRRAD_SHADER_NAME)
    {
        static SimpleComputePipelineRegistrar ENVMAPTOIRRAD_SHADER_PIPELINE_REGISTER(this->getResourceName());
    }
};

template EnvMapToIrradiance<4, 4, 1>;
template EnvMapToIrradiance<16, 16, 1>;

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(EnvMapToIrradiance, <ExpandArgs(uint32 SizeX, uint32 SizeY, uint32 SizeZ)>, <ExpandArgs(SizeX, SizeY, SizeZ)>)

