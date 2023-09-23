/*!
 * \file WindowsThreadingFunctions.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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

    static void setThreadName(const TChar *name, PlatformHandle threadHandle);

    static String getThreadName(PlatformHandle threadHandle);
    static String getCurrentThreadName();
    static PlatformHandle getCurrentThreadHandle();

    static bool setThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx, PlatformHandle threadHandle);
    static bool setThreadGroupAffinity(uint16 grpIdx, uint64 affinityMask, PlatformHandle threadHandle);

    static void sleep(int64 msTicks);

    static void printSystemThreadingInfo();
    static SystemProcessorsInfo getSystemProcessorInfo();
    static SystemProcessorsCacheInfo getProcessorCacheInfo();
};

namespace GPlatformThreadingFunctions
{
using PlatformThreadingFunctions = WindowsThreadingFunctions;
}