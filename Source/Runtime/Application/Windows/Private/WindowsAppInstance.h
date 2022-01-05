/*!
 * \file WindowsAppInstance.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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