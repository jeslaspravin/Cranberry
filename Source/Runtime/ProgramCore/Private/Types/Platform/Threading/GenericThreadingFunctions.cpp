/*!
 * \file GenericThreadingFunctions.cpp
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/Threading/GenericThreadingFunctions.h"
#include "Types/Delegates/Delegate.h"
#include "Logger/Logger.h"

#include <thread>

namespace ThreadingHelpers
{

void INTERNAL_printSystemThreadingInfo(SystemProcessorsInfo processorInfo, SystemProcessorsCacheInfo cacheInfo)
{
    const TChar *fmtStrProcInfo = TCHAR(
    "\n+=======================================+\n"\
    "|PROCESSOR INFO:                           \n"\
    "|    Logical group count: %u               \n"\
    "|    Physical processor count: %u          \n"\
    "|    Core count: %u                        \n"\
    "|    Logical processor count: %u           \n"\
    "+=======================================+"
    );
    LOG("PlatformThreading", fmtStrProcInfo, processorInfo.logicalGroupsCount, processorInfo.physicalProcessorCount, processorInfo.coresCount,
        processorInfo.logicalProcessorsCount);

    const TChar *fmtStrCacheInfo = TCHAR(
    "\n+========================================================================================+\n"\
    "|PROCESSOR CACHE INFO:                                                                      \n"\
    "|    Cache Line size: %u                                                                    \n"\
    "|    L1:                                                                                    \n"\
    "|%s                                                                                         \n"\
    "|    L2:                                                                                    \n"\
    "|%s                                                                                         \n"\
    "|    L3:                                                                                    \n"\
    "|%s                                                                                         \n"\
    "+========================================================================================+"
    );
    auto printCacheInfo = [](SystemProcessorsCacheInfo::CacheUnit cacheUnit, uint32 puShareCount, uint32 logicalProcessorCount) -> String
    {
        if (cacheUnit.bSplitDesign)
        {
            uint32 totalCacheSize
                = ((cacheUnit.caches.iCacheByteSize + cacheUnit.caches.dCacheByteSize + cacheUnit.caches.tCacheByteSize) / puShareCount)
                  * logicalProcessorCount;
            const TChar *localFmtStr = TCHAR(
            "        Cache Unit Size: [Instruction:%ubytes Data:%ubytes Trace:%ubytes]\n"\
            "|        Total Cache Size: %ubytes"
            );
            return StringFormat::format(
                localFmtStr, cacheUnit.caches.iCacheByteSize, cacheUnit.caches.dCacheByteSize, cacheUnit.caches.tCacheByteSize, totalCacheSize
            );
        }
        else
        {
            uint32 totalCacheSize = (cacheUnit.uCacheByteSize / puShareCount) * logicalProcessorCount;
            const TChar *localFmtStr = TCHAR(
            "        Cache Unit Size: %ubytes\n"\
            "|        Total Cache Size: %ubytes"
            );
            return StringFormat::format(localFmtStr, cacheUnit.uCacheByteSize, totalCacheSize);
        }
    };
    LOG("PlatformThreading", fmtStrCacheInfo, cacheInfo.cacheLineSize,
        printCacheInfo(cacheInfo.unitL1ByteSize, cacheInfo.puSharingL1, processorInfo.logicalProcessorsCount),
        printCacheInfo(cacheInfo.unitL2ByteSize, cacheInfo.puSharingL2, processorInfo.logicalProcessorsCount),
        printCacheInfo(cacheInfo.unitL3ByteSize, cacheInfo.puSharingL3, processorInfo.logicalProcessorsCount));
}

void sleep(int64 msTicks) { std::this_thread::sleep_for(std::chrono::milliseconds(msTicks)); }

struct ThreadExitListener
{
    SimpleDelegate callbacks;

    ThreadExitListener() = default;
    ~ThreadExitListener() { callbacks.invoke(); }

    static ThreadExitListener &getListener()
    {
        static thread_local ThreadExitListener listener;
        return listener;
    }
};

void atThreadExit(Function<void> callback) { ThreadExitListener::getListener().callbacks.bindStatic(std::move(callback)); }

void atThreadExit(LambdaFunction<void> callback) { ThreadExitListener::getListener().callbacks.bindLambda(std::move(callback)); }

} // namespace ThreadingHelpers