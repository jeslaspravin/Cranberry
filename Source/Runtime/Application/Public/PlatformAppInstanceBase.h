/*!
 * \file PlatformAppInstanceBase.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "Types/Platform/PlatformTypes.h"

class APPLICATION_EXPORT PlatformAppInstanceBase
{
public:
    virtual InstanceHandle getPlatformAppInstance() const = 0;
};
