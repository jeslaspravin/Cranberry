/*!
 * \file DeferredDeleter.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineRendererExports.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/Threading/SyncPrimitives.h"
#include "Types/Time.h"
#include "Types/Delegates/Delegate.h"

#include <vector>

class GraphicsResource;

enum class EDeferredDelStrategy
{
    FrameCount,     // Tries deleting a resource after given frame count
    SwapchainCount, // Tries deleting after swapchain count of frame
    TimePeriod,     // Tries to delete after a time period
    Immediate
};

class ENGINERENDERER_EXPORT DeferredDeleter
{
public:
    struct DeferringData
    {
        GraphicsResource *resource = nullptr;
        // Deleter for custom deferred clearing resource
        SimpleSingleCastDelegate deleter;

        // Defer duration in time tick or frame count
        TickRep deferDuration;
        // Will be start tick in time mode or frame count elapsed
        TickRep elapsedDuration;
        EDeferredDelStrategy strategy;
    };

private:
    std::vector<DeferringData> deletingResources[2];
    uint8 readAtIdx = 0; // No need to be atomic since spin lock protects this

    // Used in case when clearing all, clears an indirect resource which in turn adds to defer delete
    std::atomic_flag bClearing;
    CBESpinLock deleteEmplaceLock;

public:
    void deferDelete(DeferringData &&deferringInfo);
    void update();
    // Clears and deletes any pending resources
    void clear();

private:
    FORCE_INLINE void deleteResource(const DeferringData &deferredResData);
    FORCE_INLINE uint8 getWritingIdx() const { return (readAtIdx + 1) % ARRAY_LENGTH(deletingResources); }
    FORCE_INLINE void swapReadWriteIdx();
};