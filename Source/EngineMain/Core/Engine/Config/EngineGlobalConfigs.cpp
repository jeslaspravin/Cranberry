#include "EngineGlobalConfigs.h"

namespace EngineSettings
{
    EngineGlobalConfig<Size2D> screenSize(Size2D(1280,720));
    EngineGlobalConfig<Size2D> surfaceSize;
    EngineGlobalConfig<bool> fullscreenMode(false);
    EngineGlobalConfig<bool> enableVsync(false);
}