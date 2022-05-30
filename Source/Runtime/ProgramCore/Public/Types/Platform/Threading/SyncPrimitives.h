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

#include "Types/CoreDefines.h"

#include <atomic>

class CBESpinLock
{
private:
    std::atomic_flag flag;

public:
    FORCE_INLINE void lock() noexcept
    {
        while (flag.test_and_set(std::memory_order::acq_rel))
        {}
    }
    FORCE_INLINE bool try_lock() noexcept { return !flag.test(std::memory_order::acquire); }
    FORCE_INLINE void unlock() noexcept { flag.clear(std::memory_order::release); }
};