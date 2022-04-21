/*!
 * \file DeferredDeleter.cpp
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/DeferredDeleter.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"

FORCE_INLINE void DeferredDeleter::deleteResource(GraphicsResource *res)
{
    res->release();
    delete res;
}

void DeferredDeleter::deferDelete(DeferringData &&deferringInfo)
{
    if (bClearing || deferringInfo.strategy == EDeferredDelStrategy::Immediate)
    {
        deleteResource(deferringInfo.resource);
        return;
    }

    for (const DeferringData &res : deletingResources)
    {
        if (res.resource == deferringInfo.resource)
        {
            return;
        }
    }
    deletingResources.emplace_back(std::forward<DeferringData>(deferringInfo));
}

void DeferredDeleter::update()
{
    if (deletingResources.empty())
    {
        return;
    }

    auto newEnd = std::remove_if(
        deletingResources.begin(), deletingResources.end(),
        [this](DeferringData &res)
        {
            uint32 references = 0;
            if (res.resource->getType()->isChildOf(MemoryResource::staticType()))
            {
                references = static_cast<MemoryResource *>(res.resource)->refCount();
            }
            else if (res.resource->getType()->isChildOf(ShaderParameters::staticType()))
            {
                references = static_cast<ShaderParameters *>(res.resource)->refCount();
            }
            else
            {
                alertIf(false, "Unsupported type(%s) for deferred deletion", res.resource->getType()->getName());
                deleteResource(res.resource);
                return true;
            }

            // Somewhere reference is acquired again so remove it from list
            if (references > 0)
            {
                return true;
            }

            TickRep currentTimeTick = Time::timeNow();
            bool bRemove = false;
            switch (res.strategy)
            {
            case EDeferredDelStrategy::FrameCount:
            case EDeferredDelStrategy::SwapchainCount:
                if (res.deferDuration == res.elapsedDuration)
                {
                    deleteResource(res.resource);
                    bRemove = true;
                }
                else
                {
                    res.elapsedDuration++;
                }
                break;
            case EDeferredDelStrategy::TimePeriod:
                if (res.deferDuration < (currentTimeTick - res.elapsedDuration))
                {
                    deleteResource(res.resource);
                    bRemove = true;
                }
                break;
            case EDeferredDelStrategy::Immediate:
            default:
                break;
            }
            return bRemove;
        }
    );
    deletingResources.erase(newEnd, deletingResources.end());
}

void DeferredDeleter::clear()
{
    bClearing = true;
    for (const DeferringData &res : deletingResources)
    {
        deleteResource(res.resource);
    }
    deletingResources.clear();
}
