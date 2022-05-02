/*!
 * \file PlatformFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Config/ProgramVarTypes.h"
#include "GenericPlatformFunctions.h"

#if PLATFORM_WINDOWS

#include "WindowsPlatformFunctions.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif

enum class EndianType
{
    Big,
    Little
};

class PROGRAMCORE_EXPORT PlatformEndian : public ProgramConstant<EndianType>
{
public:
    PlatformEndian();

    bool isBigEndian() const;
    bool isLittleEndian() const;
};

using PlatformFunctions = GPlatformFunctions::PlatformFunctions;

namespace GPlatformConfigs
{
PROGRAMCORE_EXPORT extern PlatformEndian PLATFORM_ENDIAN;
}