/*!
 * \file WindowsThreadingFunctions.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/Threading/GenericThreadingFunctions.h"

class PROGRAMCORE_EXPORT WindowsThreadingFunctions : public GenericThreadingFunctions<WindowsThreadingFunctions>
{
public:
    static bool createTlsSlot(uint32 &outSlot);
    static void releaseTlsSlot(uint32 slot);
    static bool setTlsSlotValue(uint32 slot, void *value);

    static void *getTlsSlotValue(uint32 slot);

    static void setThreadName(const TChar *name, void *threadHandle);
    static void setCurrentThreadName(const TChar *name);

    static String getThreadName(void *threadHandle);
    static String getCurrentThreadName();
    static void *getCurrentThreadHandle();
};

namespace GPlatformThreadingFunctions
{
using PlatformThreadingFunctions = WindowsThreadingFunctions;
}