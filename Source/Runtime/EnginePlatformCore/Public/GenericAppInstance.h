#pragma once

#include "Types/CoreTypes.h"
#include "String/String.h"
#include "EnginePlatformCoreExports.h"

class AssetManager;
class WindowManager;


struct ENGINEPLATFORMCORE_EXPORT GenericAppInstance 
{
    String applicationName;
    int32 headVersion;
    int32 majorVersion;
    int32 subVersion;
    String cmdLine;

    WindowManager* appWindowManager;
    AssetManager* assetManager;
};