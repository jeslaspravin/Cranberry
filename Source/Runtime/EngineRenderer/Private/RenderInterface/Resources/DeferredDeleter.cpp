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
    if (bClearing.test(std::memory_order::acquire) || deferringInfo.strategy == EDeferredDelStrategy::Immediate)
    {
        deleteResource(deferringInfo.resource);
        return;
    }

    std::scoped_lock<CBESpinLock> writeLock(deleteEmplaceLock);
    // Not necessary to do the below check as double delete must be handled differently below is not effective way
    // for (const DeferringData &res : deletingResources[getWritingIdx()])
    //{
    //    if (res.resource == deferringInfo.resource)
    //    {
    //        return;
    //    }
    //}
    deletingResources[getWritingIdx()].emplace_back(std::forward<DeferringData>(deferringInfo));
}

void DeferredDeleter::update()
{
    if (deletingResources[readAtIdx].empty())
    {
        return;
    }

    auto newEnd = std::remove_if(
        deletingResources[readAtIdx].begin(), deletingResources[readAtIdx].end(),
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
    // Copy the survived elements alone and clear the reading list
    std::vector<DeferringData> pendingDeleteResources(deletingResources[readAtIdx].begin(), newEnd);
    deletingResources[readAtIdx].clear();

    // Now swap the read write buffers
    deleteEmplaceLock.lock();
    readAtIdx = getWritingIdx();
    deleteEmplaceLock.unlock();

    // now append the pendingDeletes to new read buffer
    deletingResources[readAtIdx].insert(deletingResources[readAtIdx].end(), pendingDeleteResources.cbegin(), pendingDeleteResources.cend());
}

void DeferredDeleter::clear()
{
    bClearing.test_and_set(std::memory_order::release);
    // Just wait until thread that is trying to insert into deletingResources is finished. We will not be in update() for sure as that and clear
    // will be called from render thread
    deleteEmplaceLock.lock();
    deleteEmplaceLock.unlock();
    for (uint32 i = 0; i < ARRAY_LENGTH(deletingResources); ++i)
    {
        for (const DeferringData &res : deletingResources[i])
        {
            deleteResource(res.resource);
        }
        deletingResources[i].clear();
    }
}
