/*!
 * \file ApplicationInstance.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ApplicationInstance.h"

ApplicationInstance::ApplicationInstance(const AppInstanceCreateInfo& createInfo)
    : applicationName(createInfo.applicationName)
    , majorVersion(createInfo.majorVersion)
    , minorVersion(createInfo.minorVersion)
    , patchVersion(createInfo.patchVersion)
    , cmdLine(createInfo.cmdLine)
    , fontManager(InitType_DefaultInit)
{}
