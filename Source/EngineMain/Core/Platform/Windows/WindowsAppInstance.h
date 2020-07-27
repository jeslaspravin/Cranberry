#pragma once
#include "../GenericAppInstance.h"
#include "WindowsCommonHeaders.h"

struct WindowsAppInstance : public GenericAppInstance {
    HINSTANCE windowsInstance;
    
};



namespace GPlatformInstances {
    typedef WindowsAppInstance PlatformAppInstance;
}