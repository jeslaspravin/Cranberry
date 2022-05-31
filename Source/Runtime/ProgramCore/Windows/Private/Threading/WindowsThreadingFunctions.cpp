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

bool WindowsThreadingFunctions::createTlsSlot(uint32 &outSlot)
{
    uint32 slotIdx = ::TlsAlloc();
    outSlot = slotIdx;
    return (slotIdx != TLS_OUT_OF_INDEXES);
}

void WindowsThreadingFunctions::releaseTlsSlot(uint32 slot) { ::TlsFree(slot); }

bool WindowsThreadingFunctions::setTlsSlotValue(uint32 slot, void *value) { return !!::TlsSetValue(slot, value); }

void *WindowsThreadingFunctions::getTlsSlotValue(uint32 slot) { return ::TlsGetValue(slot); }

void WindowsThreadingFunctions::setThreadName(const TChar *name, void *threadHandle)
{
    ::SetThreadDescription(threadHandle, TCHAR_TO_WCHAR(name));
}

void WindowsThreadingFunctions::setCurrentThreadName(const TChar *name) { setThreadName(name, getCurrentThreadHandle()); }

String WindowsThreadingFunctions::getThreadName(void *threadHandle)
{
    WChar *threadName;
    ::GetThreadDescription(threadHandle, &threadName);
    return WCHAR_TO_TCHAR(threadName);
}

String WindowsThreadingFunctions::getCurrentThreadName() { return getThreadName(getCurrentThreadHandle()); }

void *WindowsThreadingFunctions::getCurrentThreadHandle() { return ::GetCurrentThread(); }
