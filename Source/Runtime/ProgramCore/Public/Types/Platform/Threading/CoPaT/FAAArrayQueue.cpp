/*!
 * \file FAAArrayQueue.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "FAAArrayQueue.hpp"

COPAT_NS_INLINED
namespace copat
{

#if COPAT_ENABLE_QUEUE_ALLOC_TRACKING
QueueNodeAllocTracker &getNodeAllocsTracker() noexcept
{
    static QueueNodeAllocTracker tracker;
    return tracker;
}
#endif // COPAT_ENABLE_QUEUE_ALLOC_TRACKING

} // namespace copat
