/*!
 * \file GenericThreadingFunctions.cpp
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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
    LOG("PlatformThreading",
        "\n+=======================================+\n"
        "|PROCESSOR INFO:                           \n"
        "|    Logical group count: {}               \n"
        "|    Physical processor count: {}          \n"
        "|    Core count: {}                        \n"
        "|    Logical processor count: {}           \n"
        "+=======================================+",
        processorInfo.logicalGroupsCount, processorInfo.physicalProcessorCount, processorInfo.coresCount, processorInfo.logicalProcessorsCount);

    auto printCacheInfo = [](SystemProcessorsCacheInfo::CacheUnit cacheUnit, uint32 puShareCount, uint32 logicalProcessorCount) -> String
    {
        if (cacheUnit.bSplitDesign)
        {
            uint32 totalCacheSize
                = ((cacheUnit.caches.iCacheByteSize + cacheUnit.caches.dCacheByteSize + cacheUnit.caches.tCacheByteSize) / puShareCount)
                  * logicalProcessorCount;
            return STR_FORMAT(
                TCHAR(
            "        Cache Unit Size: [Instruction:{}bytes Data:{}bytes Trace:{}bytes]\n"\
            "|        Total Cache Size: {}bytes"
            ), cacheUnit.caches.iCacheByteSize, cacheUnit.caches.dCacheByteSize, cacheUnit.caches.tCacheByteSize, totalCacheSize
            );
        }
        else
        {
            uint32 totalCacheSize = (cacheUnit.uCacheByteSize / puShareCount) * logicalProcessorCount;
            return STR_FORMAT(TCHAR(
            "        Cache Unit Size: {}bytes\n"\
            "|        Total Cache Size: {}bytes"
            ), cacheUnit.uCacheByteSize, totalCacheSize);
        }
    };
    LOG("PlatformThreading",
        "\n+========================================================================================+\n"
        "|PROCESSOR CACHE INFO:                                                                      \n"
        "|    Cache Line size: {}                                                                    \n"
        "|    L1:                                                                                    \n"
        "|{}                                                                                         \n"
        "|    L2:                                                                                    \n"
        "|{}                                                                                         \n"
        "|    L3:                                                                                    \n"
        "|{}                                                                                         \n"
        "+========================================================================================+",
        cacheInfo.cacheLineSize, printCacheInfo(cacheInfo.unitL1ByteSize, cacheInfo.puSharingL1, processorInfo.logicalProcessorsCount),
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