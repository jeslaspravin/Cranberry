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

    FORCE_INLINE static void setThreadName(const TChar *name, void *threadHandle) { PlatformClass::setThreadName(name, threadHandle); }
    FORCE_INLINE static void setCurrentThreadName(const TChar *name) { PlatformClass::setCurrentThreadName(name); }

    FORCE_INLINE static String getThreadName(void *threadHandle) { return PlatformClass::getThreadName(threadHandle); }
    FORCE_INLINE static String getCurrentThreadName() { return PlatformClass::getCurrentThreadName(); }
    FORCE_INLINE static void *getCurrentThreadHandle() { return PlatformClass::getCurrentThreadHandle(); }
};