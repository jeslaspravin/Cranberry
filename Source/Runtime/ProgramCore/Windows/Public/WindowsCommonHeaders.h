/*!
 * \file WindowsCommonHeaders.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"

#if USING_WIDE_UNICODE
#define UNICODE 1
#endif

#define NOMINMAX
#include <windows.h>