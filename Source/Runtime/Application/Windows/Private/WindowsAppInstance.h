#pragma once
#include "PlatformAppInstanceBase.h"

class WindowsAppInstance : public PlatformAppInstanceBase 
{
private:
    void* windowsInstance;
public:
    WindowsAppInstance(void* appPlatformInstance)
        : windowsInstance(appPlatformInstance)
    {}

    void* getPlatformAppInstance() const override;

};

namespace GPlatformInstances 
{
    typedef WindowsAppInstance PlatformAppInstance;
}