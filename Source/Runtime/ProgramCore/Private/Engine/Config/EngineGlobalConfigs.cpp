/*!
 * \file EngineGlobalConfigs.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Engine/Config/EngineGlobalConfigs.h"

namespace EngineSettings
{
ProgramGlobalVar<Size2D> screenSize(Size2D(1280, 720));
ProgramGlobalVar<Size2D> surfaceSize;
ProgramGlobalVar<bool> fullscreenMode(false);
ProgramGlobalVar<bool> enableVsync(true);

ProgramGlobalVar<uint32> minSamplingMipLevel(10u);
ProgramGlobalVar<uint32> maxPrefilteredCubeMiplevels(8u);
ProgramGlobalVar<uint32> maxEnvMapSize(1024u);

ProgramGlobalVar<int32> pcfKernelSize(3);
ProgramGlobalVar<int32> pointPcfKernelSize(4);

ProgramGlobalVar<uint32> globalSampledTexsSize(128u);
} // namespace EngineSettings