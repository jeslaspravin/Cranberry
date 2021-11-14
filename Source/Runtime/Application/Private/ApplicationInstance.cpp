#include "ApplicationInstance.h"

ApplicationInstance::ApplicationInstance(const AppInstanceCreateInfo& createInfo)
    : applicationName(createInfo.applicationName)
    , majorVersion(createInfo.majorVersion)
    , minorVersion(createInfo.minorVersion)
    , patchVersion(createInfo.patchVersion)
    , cmdLine(createInfo.cmdLine)
{}
