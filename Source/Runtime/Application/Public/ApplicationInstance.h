#pragma once

#include "Types/CoreTypes.h"
#include "String/String.h"
#include "ApplicationExports.h"

class PlatformAppInstanceBase;

struct AppInstanceCreateInfo
{
    String applicationName;

    int32 majorVersion;
    int32 minorVersion;
    int32 patchVersion;
    // This cmdLine will be used as reference inside ProgramCmdLine
    String cmdLine;

    void* platformAppHandle;
};

class APPLICATION_EXPORT ApplicationInstance 
{
private:
    String applicationName;
    
    int32 majorVersion;
    int32 minorVersion;
    int32 patchVersion;
    String cmdLine;

public:
    ApplicationInstance() = default;
    ApplicationInstance(const AppInstanceCreateInfo& createInfo);

    PlatformAppInstanceBase* platformApp;

    const String& getAppName() const { return applicationName; }
    void getVersion(int32& majorVer, int32& minorVer, int32& patchVer) const 
    { 
        majorVer = majorVersion;
        minorVer = minorVersion;
        patchVer = patchVersion;
    }
};