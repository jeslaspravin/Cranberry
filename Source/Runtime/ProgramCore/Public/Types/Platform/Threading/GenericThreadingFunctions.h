/*!
 * \file GenericThreadingFunctions.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "Types/Platform/PlatformTypes.h"

struct SystemProcessorsCacheInfo
{
    struct CacheUnit
    {
        union
        {
            uint64 __val[2] = { 0, 0 };

            struct
            {
                uint32 dCacheByteSize;
                uint32 iCacheByteSize;
                uint32 tCacheByteSize;
            };
            uint32 uCacheByteSize;
        };
        bool bSplitDesign;
    };
    CacheUnit unitL1ByteSize;
    uint32 puSharingL1 = 0;
    CacheUnit unitL2ByteSize;
    uint32 puSharingL2 = 0;
    CacheUnit unitL3ByteSize;
    uint32 puSharingL3 = 0;
    uint32 cacheLineSize = 0;
};

struct SystemProcessorsInfo
{
    uint32 physicalProcessorCount = 0;
    uint32 coresCount = 0;
    uint32 logicalProcessorsCount = 0;
    uint32 logicalGroupsCount = 0;
};

extern void INTERNAL_printSystemThreadingInfo(SystemProcessorsInfo processorInfo, SystemProcessorsCacheInfo cacheInfo);

template <typename PlatformClass>
class GenericThreadingFunctions
{
private:
    GenericThreadingFunctions() = default;

public:
    FORCE_INLINE static bool createTlsSlot(uint32 &outSlot) { return PlatformClass::createTlsSlot(outSlot); }
    FORCE_INLINE static void releaseTlsSlot(uint32 slot) { PlatformClass::releaseTlsSlot(slot); }
    FORCE_INLINE static bool setTlsSlotValue(uint32 slot, void *value) { return PlatformClass::setTlsSlotValue(slot, value); }

    FORCE_INLINE static void *getTlsSlotValue(uint32 slot) { return PlatformClass::getTlsSlotValue(slot); }

    FORCE_INLINE static void setThreadName(const TChar *name, PlatformHandle threadHandle) { PlatformClass::setThreadName(name, threadHandle); }
    FORCE_INLINE static void setCurrentThreadName(const TChar *name) { setThreadName(name, getCurrentThreadHandle); }

    FORCE_INLINE static String getThreadName(PlatformHandle threadHandle) { return PlatformClass::getThreadName(threadHandle); }
    FORCE_INLINE static String getCurrentThreadName() { return PlatformClass::getCurrentThreadName(); }
    FORCE_INLINE static PlatformHandle getCurrentThreadHandle() { return PlatformClass::getCurrentThreadHandle(); }

    FORCE_INLINE static void getCoreCount(uint32 &outCoreCount, uint32 &outLogicalProcessorCount)
    {
        SystemProcessorsInfo processorsInfo = getSystemProcessorInfo();
        outCoreCount = processorsInfo.coresCount;
        outLogicalProcessorCount = processorsInfo.logicalProcessorsCount;
    }
    FORCE_INLINE static bool setThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx, void *threadHandle)
    {
        return PlatformClass::setThreadProcessor(coreIdx, logicalProcessorIdx, threadHandle);
    }
    FORCE_INLINE static bool setCurrentThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx)
    {
        return setThreadProcessor(coreIdx, logicalProcessorIdx, getCurrentThreadHandle());
    }

    // Miscellaneous
    FORCE_INLINE static void printSystemThreadingInfo() { PlatformClass::printSystemThreadingInfo(); }
    FORCE_INLINE static SystemProcessorsInfo getSystemProcessorInfo() { return PlatformClass::getSystemProcessorInfo(); }
    FORCE_INLINE static SystemProcessorsCacheInfo getProcessorCacheInfo() { return PlatformClass::getProcessorCacheInfo(); }
};
