#include "Engine/Config/EngineGlobalConfigs.h"

namespace EngineSettings
{
    EngineGlobalConfig<Size2D> screenSize(Size2D(1280,720));
    EngineGlobalConfig<Size2D> surfaceSize;
    EngineGlobalConfig<bool> fullscreenMode(false);
    EngineGlobalConfig<bool> enableVsync(true);

    EngineGlobalConfig<uint32> minSamplingMipLevel(10u);
    EngineGlobalConfig<uint32> maxPrefilteredCubeMiplevels(8u);
    EngineGlobalConfig<uint32> maxEnvMapSize(1024u);

    EngineGlobalConfig<int32> pcfKernelSize(3);
    EngineGlobalConfig<int32> pointPcfKernelSize(4);

    EngineGlobalConfig<uint32> globalSampledTexsSize(128u);
}