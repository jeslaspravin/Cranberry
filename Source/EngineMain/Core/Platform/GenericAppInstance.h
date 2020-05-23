#pragma once

#include "../Platform/PlatformTypes.h"
#include "../String/String.h"
#include "../Engine/WindowManager.h"
#include "../../Assets/AssetsManager.h"

class GenericAppWindow;


struct GenericAppInstance {

    String applicationName;
    int32 headVersion;
    int32 majorVersion;
    int32 subVersion;
    String cmdLine;

    WindowManager appWindowManager;
    AssetManager assetManager;
    const InputSystem* inputSystem() const;
};