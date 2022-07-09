/*!
 * \file WindowsThreadingFunctions.cpp
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Threading/WindowsThreadingFunctions.h"
#include "WindowsCommonHeaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

bool WindowsThreadingFunctions::createTlsSlot(uint32 &outSlot)
{
    uint32 slotIdx = ::TlsAlloc();
    outSlot = slotIdx;
    return (slotIdx != TLS_OUT_OF_INDEXES);
}

void WindowsThreadingFunctions::releaseTlsSlot(uint32 slot) { ::TlsFree(slot); }

bool WindowsThreadingFunctions::setTlsSlotValue(uint32 slot, void *value) { return !!::TlsSetValue(slot, value); }

void *WindowsThreadingFunctions::getTlsSlotValue(uint32 slot) { return ::TlsGetValue(slot); }

void WindowsThreadingFunctions::setThreadName(const TChar *name, PlatformHandle threadHandle)
{
    ::SetThreadDescription((HANDLE)threadHandle, TCHAR_TO_WCHAR(name));
}

String WindowsThreadingFunctions::getThreadName(PlatformHandle threadHandle)
{
    WChar *threadName;
    if (SUCCEEDED(::GetThreadDescription((HANDLE)threadHandle, &threadName)))
    {
        String outStr = WCHAR_TO_TCHAR(threadName);
        ::LocalFree(threadName);
        return std::move(outStr);
    }
    return TCHAR("");
}

String WindowsThreadingFunctions::getCurrentThreadName() { return getThreadName(getCurrentThreadHandle()); }

PlatformHandle WindowsThreadingFunctions::getCurrentThreadHandle() { return ::GetCurrentThread(); }

bool WindowsThreadingFunctions::setThreadProcessor(uint32 coreIdx, uint32 logicalProcessorIdx, PlatformHandle threadHandle)
{
    uint32 coreCount, logicalProcCount;
    getCoreCount(coreCount, logicalProcCount);

    const uint32 hyperthread = logicalProcCount / coreCount;
    debugAssert(hyperthread > logicalProcessorIdx);
    const uint32 coreAffinityShift = coreIdx * hyperthread + logicalProcessorIdx;
    const uint32 groupIndex = coreAffinityShift / 64;
    const uint64 groupAffinityMask = 1ull << (coreAffinityShift % 64);

    // Need to zero initialize for ::SetThreadGroupAffinity to succeed!
    ::GROUP_AFFINITY grpAffinity = {};
    grpAffinity.Group = WORD(groupIndex);
    grpAffinity.Mask = groupAffinityMask;

    return !!::SetThreadGroupAffinity((HANDLE)threadHandle, &grpAffinity, nullptr);
}

void WindowsThreadingFunctions::sleep(int64 msTicks) 
{
    ::Sleep(DWORD(msTicks));
}

template <typename T>
void windowsLogicalProcessorInfoVisitor(T &&func, std::vector<uint8> &buffer, LOGICAL_PROCESSOR_RELATIONSHIP processorRelation)
{
    DWORD processorsInfoLen = 0;
    if (!::GetLogicalProcessorInformationEx(processorRelation, nullptr, &processorsInfoLen) && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        buffer.resize(processorsInfoLen);
        ::GetLogicalProcessorInformationEx(processorRelation, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data(), &processorsInfoLen);

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();
        for (uint32 i = 0; i < processorsInfoLen; i += procInfo->Size)
        {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buffer.data() + i);
            if (procInfo->Size == 0 || procInfo->Relationship != processorRelation)
            {
                continue;
            }

            func(procInfo);
        }
    }
}

SystemProcessorsInfo WindowsThreadingFunctions::getSystemProcessorInfo()
{
    SystemProcessorsInfo processorInfo;

    std::vector<uint8> buffer;
    uint32 activeProcessorsCount = 0;
    /**
     * Each Relation provides all the logical processors and its group related to it and the related component's properties
     * Example Group lists all processors active under a group
     * Cache lists the cache's property and all the processor's(In its group) that shares this cache
     * ProcessorCore lists for each core and its group and logical processors
     */
    windowsLogicalProcessorInfoVisitor(
        [&processorInfo, &activeProcessorsCount](const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo)
        {
            processorInfo.logicalGroupsCount = procInfo->Group.ActiveGroupCount;
            for (uint32 i = 0; i < procInfo->Group.ActiveGroupCount; ++i)
            {
                activeProcessorsCount += procInfo->Group.GroupInfo[i].ActiveProcessorCount;
            }
        },
        buffer, RelationGroup
    );

    windowsLogicalProcessorInfoVisitor(
        [&processorInfo](const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo) { processorInfo.physicalProcessorCount++; }, buffer,
        RelationProcessorPackage
    );

    windowsLogicalProcessorInfoVisitor(
        [&processorInfo](const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo)
        {
            processorInfo.coresCount++;
            for (int i = 0; i < procInfo->Processor.GroupCount; ++i)
            {
                processorInfo.logicalProcessorsCount += uint32(::__popcnt64(procInfo->Processor.GroupMask[i].Mask));
            }
        },
        buffer, RelationProcessorCore
    );
    debugAssert(processorInfo.logicalProcessorsCount == activeProcessorsCount);

    return processorInfo;
}
SystemProcessorsCacheInfo WindowsThreadingFunctions::getProcessorCacheInfo()
{
    auto setupCacheInfo
        = [](const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo, SystemProcessorsCacheInfo::CacheUnit &cacheUnit, uint32 &puShareCount)
    {
        if (puShareCount == 0)
        {
            puShareCount = uint32(::__popcnt64(procInfo->Cache.GroupMask.Mask));
        }
        else
        {
            debugAssert(puShareCount == uint32(::__popcnt64(procInfo->Cache.GroupMask.Mask)));
        }
        switch (procInfo->Cache.Type)
        {
        case CacheUnified:
        {
            if (cacheUnit.uCacheByteSize == 0)
            {
                cacheUnit.bSplitDesign = false;
                cacheUnit.uCacheByteSize = procInfo->Cache.CacheSize;
            }
            else
            {
                debugAssert(cacheUnit.bSplitDesign == false && cacheUnit.uCacheByteSize == procInfo->Cache.CacheSize);
            }
            break;
        }
        case CacheInstruction:
            if (cacheUnit.iCacheByteSize == 0)
            {
                cacheUnit.bSplitDesign = true;
                cacheUnit.iCacheByteSize = procInfo->Cache.CacheSize;
            }
            else
            {
                debugAssert(cacheUnit.bSplitDesign == true && cacheUnit.iCacheByteSize == procInfo->Cache.CacheSize);
            }
            break;
        case CacheData:
            if (cacheUnit.dCacheByteSize == 0)
            {
                cacheUnit.bSplitDesign = true;
                cacheUnit.dCacheByteSize = procInfo->Cache.CacheSize;
            }
            else
            {
                debugAssert(cacheUnit.bSplitDesign == true && cacheUnit.dCacheByteSize == procInfo->Cache.CacheSize);
            }
            break;
        case CacheTrace:
            if (cacheUnit.tCacheByteSize == 0)
            {
                cacheUnit.bSplitDesign = true;
                cacheUnit.tCacheByteSize = procInfo->Cache.CacheSize;
            }
            else
            {
                debugAssert(cacheUnit.bSplitDesign == true && cacheUnit.tCacheByteSize == procInfo->Cache.CacheSize);
            }
            break;
        }
    };

    SystemProcessorsCacheInfo cacheInfo;
    std::vector<uint8> buffer;
    windowsLogicalProcessorInfoVisitor(
        [&cacheInfo, &setupCacheInfo](const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *procInfo)
        {
            if (procInfo->Cache.CacheSize == 0)
                return;

            if (cacheInfo.cacheLineSize == 0)
            {
                cacheInfo.cacheLineSize = procInfo->Cache.LineSize;
            }
            else
            {
                debugAssert(cacheInfo.cacheLineSize == procInfo->Cache.LineSize);
            }
            switch (procInfo->Cache.Level)
            {
            case 1:
            {
                setupCacheInfo(procInfo, cacheInfo.unitL1ByteSize, cacheInfo.puSharingL1);
                break;
            }
            case 2:
            {
                setupCacheInfo(procInfo, cacheInfo.unitL2ByteSize, cacheInfo.puSharingL2);
                break;
            }
            case 3:
            {
                setupCacheInfo(procInfo, cacheInfo.unitL3ByteSize, cacheInfo.puSharingL3);
                break;
            }
            default:
                break;
            }
        },
        buffer, RelationCache
    );
    return cacheInfo;
}

void WindowsThreadingFunctions::printSystemThreadingInfo()
{
    ThreadingHelpers::INTERNAL_printSystemThreadingInfo(getSystemProcessorInfo(), getProcessorCacheInfo());
}