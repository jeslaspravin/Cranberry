/*!
 * \file GenericThreadingFunctions.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "Types/Platform/PlatformTypes.h"
#include "Reflections/Functions.h"

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
            } caches;
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

// Helper non templated functions for GenericThreadingFunctions
namespace ThreadingHelpers
{
void INTERNAL_printSystemThreadingInfo(SystemProcessorsInfo processorInfo, SystemProcessorsCacheInfo cacheInfo);

PROGRAMCORE_EXPORT void sleep(int64 msTicks);
PROGRAMCORE_EXPORT void atThreadExit(Function<void> callback);
PROGRAMCORE_EXPORT void atThreadExit(LambdaFunction<void> callback);
} // namespace ThreadingHelpers

template <typename PlatformClass>
class GenericThreadingFunctions
{
private:
    GenericThreadingFunctions() = default;

public:
    struct GroupAffinityMaskBuilder
    {
    public:
        GroupAffinityMaskBuilder()
            : mask(0)
            , groupIdx(0)
        {
            uint32 nCore, nLogicalProcs;
            getCoreCount(nCore, nLogicalProcs);
            coreNum = uint16(nCore);
            logicProcsPerCore = uint16(nLogicalProcs / nCore);
        }

        GroupAffinityMaskBuilder &setGroupFrom(uint32 coreIdx)
        {
            uint16 logicProcGlobalIdx = uint16(coreIdx * logicProcsPerCore);
            groupIdx = logicProcGlobalIdx / LOGIC_PROCS_PER_GROUP;
            return *this;
        }
        GroupAffinityMaskBuilder &setAll()
        {
            mask = ~(0ull);
            return *this;
        }
        GroupAffinityMaskBuilder &clearUpto(uint32 coreIdx, uint32 logicalProcessorIdx)
        {
            uint16 logicProcGlobalIdx = uint16(coreIdx * logicProcsPerCore + logicalProcessorIdx);
            if (isLogicProcGlobalIdxInsideGrp(logicProcGlobalIdx))
            {
                uint16 bitIdx = logicProcGlobalIdx % LOGIC_PROCS_PER_GROUP;
                uint64 bitsToClear = (1ull << bitIdx) - 1;
                CLEAR_BITS(mask, bitsToClear);
            }
            return *this;
        }

        uint16 getGroupIdx() const { return groupIdx; }
        uint64 getAffinityMask() const { return mask; }

    private:
        uint64 mask;
        uint16 groupIdx;
        uint16 coreNum;
        uint16 logicProcsPerCore;

        constexpr static const uint16 LOGIC_PROCS_PER_GROUP = 64;
        bool isLogicProcGlobalIdxInsideGrp(uint16 logicProcGlobalIdx) const
        {
            return logicProcGlobalIdx >= (groupIdx * LOGIC_PROCS_PER_GROUP)
                   && logicProcGlobalIdx < (groupIdx * LOGIC_PROCS_PER_GROUP + LOGIC_PROCS_PER_GROUP);
        }
    };

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
    FORCE_INLINE static bool setThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx, PlatformHandle threadHandle)
    {
        return PlatformClass::setThreadProcessor(coreIdx, logicalProcessorIdx, threadHandle);
    }
    FORCE_INLINE static bool setCurrentThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx)
    {
        return setThreadProcessor(coreIdx, logicalProcessorIdx, getCurrentThreadHandle());
    }
    /**
     * Each group has 64 logical processors, Each logical processor has a bit in the affinity mask.
     */
    FORCE_INLINE static bool setThreadGroupAffinity(uint16 grpIdx, uint64 affinityMask, PlatformHandle threadHandle)
    {
        return PlatformClass::setThreadGroupAffinity(grpIdx, affinityMask, threadHandle);
    }

    // Ticks in milliseconds
    FORCE_INLINE static void sleep(int64 msTicks) { ThreadingHelpers::sleep(msTicks); }
    template <typename T>
    FORCE_INLINE static void atThreadExit(T &&callback)
    {
        ThreadingHelpers::atThreadExit(std::forward<T>(callback));
    }

    // Miscellaneous
    FORCE_INLINE static void printSystemThreadingInfo() { PlatformClass::printSystemThreadingInfo(); }
    FORCE_INLINE static SystemProcessorsInfo getSystemProcessorInfo() { return PlatformClass::getSystemProcessorInfo(); }
    FORCE_INLINE static SystemProcessorsCacheInfo getProcessorCacheInfo() { return PlatformClass::getProcessorCacheInfo(); }
};
