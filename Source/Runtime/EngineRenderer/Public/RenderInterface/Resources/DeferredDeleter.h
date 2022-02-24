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

#include "Types/Time.h"
#include "Types/CoreDefines.h"
#include "EngineRendererExports.h"

#include <vector>

class GraphicsResource;

enum class EDeferredDelStrategy
{
    FrameCount, // Tries deleting a resource after given frame count
    SwapchainCount, // Tries deleting after swapchain count of frame
    TimePeriod, // Tries to delete after a time period
    Immediate
};

class ENGINERENDERER_EXPORT DeferredDeleter
{
public:
    struct DeferringData
    {
        GraphicsResource* resource;

        // Defer duration in time tick or frame count
        TickRep deferDuration;
        // Will be start tick in time mode or frame count elapsed
        TickRep elapsedDuration;
        EDeferredDelStrategy strategy;
    };

private:
    std::vector<DeferringData> deletingResources;
    // Used in case when clearing all clears an indirect resource which in turn adds to defer delete
    bool bClearing = false;
private:
    FORCE_INLINE void deleteResource(GraphicsResource* res);
public:
    void deferDelete(DeferringData&& deferringInfo);
    void update();
    // Clears and deletes any pending resources
    void clear();
};