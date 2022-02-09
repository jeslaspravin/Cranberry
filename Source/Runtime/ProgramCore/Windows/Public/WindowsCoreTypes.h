/*!
 * \file WindowsCoreTypes.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once
#include "Types/Platform/GenericPlatformCoreTypes.h"

class WindowsCoreTypes : public GenericPlatformCoreTypes<WindowsCoreTypes>
{
private:
    WindowsCoreTypes() = default;
public:
    typedef Utf16 EncodedType;
};

using PlatformCoreTypes = WindowsCoreTypes;