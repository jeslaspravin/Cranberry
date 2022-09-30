/*!
 * \file SyncPrimitives.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoPaTConfig.h"

#ifndef OVERRIDE_SPINLOCK
#include <atomic>
#endif

COPAT_NS_INLINED
namespace copat
{
#if 0 // Dead code
class WindowsFuncs
{
public:
    static bool createTlsSlot(u32 &outSlot)
    {
        u32 slotIdx = ::TlsAlloc();
        outSlot = slotIdx;
        return (slotIdx != TLS_OUT_OF_INDEXES);
    }

    static void releaseTlsSlot(u32 slot) { ::TlsFree(slot); }

    static bool setTlsSlotValue(u32 slot, void *value) { return !!::TlsSetValue(slot, value); }

    static void *getTlsSlotValue(u32 slot) { return ::TlsGetValue(slot); }

    static void setThreadName(const wchar_t *name, void *threadHandle) { ::SetThreadDescription(threadHandle, name); }
    static void setCurrentThreadName(const wchar_t *name) { ::SetThreadDescription(::GetCurrentThread(), name); }

    static std::string getCurrentThreadName()
    {
        HANDLE threadHnd = ::GetCurrentThread();
        wchar_t *threadName;
        ::GetThreadDescription(threadHnd, &threadName);

        std::string outStr;
        int32_t bufLen = ::WideCharToMultiByte(CP_UTF8, 0, threadName, -1, NULL, 0, NULL, NULL);
        outStr.resize(bufLen);
        bufLen = ::WideCharToMultiByte(CP_UTF8, 0, threadName, -1, outStr.data(), bufLen, NULL, NULL);
        return outStr;
    }
};

class WindowsCriticalSection
{
private:
    CRITICAL_SECTION cs;

public:
    WindowsCriticalSection(int spinCount = 4000) { ::InitializeCriticalSectionAndSpinCount(&cs, spinCount); }
    ~WindowsCriticalSection() { ::DeleteCriticalSection(&cs); }

    void lock() noexcept { ::EnterCriticalSection(&cs); }
    bool try_lock() noexcept { return !!::TryEnterCriticalSection(&cs); }
    void unlock() noexcept { ::LeaveCriticalSection(&cs); }

    void *getHandle() noexcept { return &cs; }
};

class WindowsConditionalVariable
{
private:
    CONDITION_VARIABLE cv;

public:
    WindowsConditionalVariable() { ::InitializeConditionVariable(&cv); }

    void wait(std::unique_lock<WindowsCriticalSection> &lock) noexcept
    {
        COPAT_ASSERT(lock);
        BOOL success = ::SleepConditionVariableCS(&cv, (PCRITICAL_SECTION)lock.mutex()->getHandle(), INFINITE);
        COPAT_ASSERT(!!success);
    }

    void notify_all() noexcept { ::WakeAllConditionVariable(&cv); }
    void notify_one() noexcept { ::WakeConditionVariable(&cv); }
};

#endif

#ifndef OVERRIDE_SPINLOCK
class SpinLock
{
private:
    std::atomic_flag flag;

public:
    void lock() noexcept
    {
        while (flag.test_and_set(std::memory_order::acq_rel))
        {}
    }
    bool try_lock() noexcept { return !flag.test(std::memory_order::acquire); }
    void unlock() noexcept { flag.clear(std::memory_order::release); }
};
#else  // OVERRIDE_SPINLOCK
using SpinLock = OVERRIDE_SPINLOCK;
#endif // OVERRIDE_SPINLOCK

} // namespace copat