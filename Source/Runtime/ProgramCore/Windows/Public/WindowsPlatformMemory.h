/*!
 * \file WindowsPlatformMemory.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/GenericPlatformMemory.h"

class PROGRAMCORE_EXPORT WindowsPlatformMemory : public GenericPlatformMemory
{};

namespace GPlatformMemory
{
typedef WindowsPlatformMemory PlatformMemory;
}