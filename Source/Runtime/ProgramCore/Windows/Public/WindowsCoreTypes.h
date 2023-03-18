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

class WindowsCoreTypes : public GenericPlatformCoreTypes
{
private:
    WindowsCoreTypes() = default;

public:
    // Right now using UTF-8 for windows as windows do not recommend UTF-16 anymore
    using TChar = AChar;
    using WCharEncodedType = Utf16;
    using EncodedType = Utf8;
};

using PlatformCoreTypes = WindowsCoreTypes;