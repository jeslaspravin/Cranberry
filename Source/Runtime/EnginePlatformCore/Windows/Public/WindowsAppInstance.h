#pragma once
#include "GenericAppInstance.h"

struct ENGINEPLATFORMCORE_EXPORT WindowsAppInstance : public GenericAppInstance 
{
    void* windowsInstance;
    
    WindowsAppInstance();
};



namespace GPlatformInstances 
{
    typedef WindowsAppInstance PlatformAppInstance;
}