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

    static String getThreadName(void *threadHandle);
    static String getCurrentThreadName();
    static void *getCurrentThreadHandle();

    static bool setThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx, void *threadHandle);

    static void printSystemThreadingInfo(); 
    static SystemProcessorsInfo getSystemProcessorInfo();
    static SystemProcessorsCacheInfo getProcessorCacheInfo();
};

namespace GPlatformThreadingFunctions
{
using PlatformThreadingFunctions = WindowsThreadingFunctions;
}