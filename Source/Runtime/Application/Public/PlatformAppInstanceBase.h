#pragma once
#include "ApplicationExports.h"

class APPLICATION_EXPORT PlatformAppInstanceBase
{
public:
    virtual void* getPlatformAppInstance() const = 0;
};

