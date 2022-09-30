/*!
 * \file BufferedResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <array>

template <typename ResourceType, uint32 Count>
requires std::is_default_constructible_v<ResourceType>
class RingBufferedResource
{
public:
    constexpr static const uint32 BUFFER_COUNT = Count;
    static_assert(BUFFER_COUNT > 1, "Only 1 buffer is not supported");

private:
    std::array<ResourceType, BUFFER_COUNT> resources;
    int32 head = 0;
    int32 tail = -1;

public:
    RingBufferedResource() = default;

    void push(ResourceType resource)
    {
        if (tail == -1)
        {
            resources[0] = resource;
            tail = 1;
            return;
        }

        debugAssertf(tail != head, "Push must be paired with pop");
        if (tail != head)
        {
            resources[tail] = resource;
            tail = (tail + 1) % BUFFER_COUNT;
        }
    }
    ResourceType peek(uint32 offset = 0) const
    {
        offset %= BUFFER_COUNT;
        if (offset < size())
        {
            return resources[(head + offset) % BUFFER_COUNT];
        }
        return {};
    }
    ResourceType pop()
    {
        debugAssertf(tail != -1, "Push must be paired with pop");
        if (tail != -1)
        {
            ResourceType res = resources[head];
            resources[head] = {};
            head = (head + 1) % BUFFER_COUNT;
            if (head == tail)
            {
                tail = -1;
                head = 0;
            }
            return res;
        }
        return {};
    }
    void reset()
    {
        for (uint32 i = 0; i != BUFFER_COUNT; ++i)
        {
            resources[i] = {};
        }
    }
    uint32 size() const
    {
        if (tail < 0)
        {
            return 0;
        }
        return head >= tail ? BUFFER_COUNT - (head - tail) : tail - head;
    }
};

template <typename ResourceType>
class SwapchainBufferedResource
{
private:
    WindowCanvasRef basedOnSwapchain;
    std::vector<ResourceType *> resources;

private:
    template <typename... ConstructParamTypes>
    void swapchainChanged(ConstructParamTypes... constructParams);

public:
    SwapchainBufferedResource() = default;

    template <typename... ConstructParamTypes>
    SwapchainBufferedResource(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams);

    template <typename... ConstructParamTypes>
    void setNewSwapchain(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams);
    const std::vector<ResourceType *> &getResources() const { return resources; }
    ResourceType *operator->() const;
    ResourceType *operator*() const;
    // resets and deletes all resources
    void reset();
    bool isValid() const;

    void init() const;
    void reinitResources() const;
    void release() const;
};

template <template <typename> typename RefCountType, typename ResourceType>
class SwapchainBufferedResource<RefCountType<ResourceType>>
{
private:
    WindowCanvasRef basedOnSwapchain;
    std::vector<RefCountType<ResourceType>> resources;

private:
    void swapchainChanged();

public:
    SwapchainBufferedResource() = default;
    SwapchainBufferedResource(WindowCanvasRef swapchainCanvas);

    void setNewSwapchain(WindowCanvasRef swapchainCanvas);
    const std::vector<RefCountType<ResourceType>> &getResources() const { return resources; }
    ResourceType *operator->() const;
    ResourceType *operator*() const;
    void set(const RefCountType<ResourceType> &resource, uint32 atIdx);
    // resets and deletes all resources
    void reset();
    bool isValid() const;

    void init() const;
    void reinitResources() const;
    void release() const;
};

//////////////////////////////////////////////////////////////////////////
/// SharedPtr specialization
//////////////////////////////////////////////////////////////////////////

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::release() const
{
    for (const RefCountType<ResourceType> &res : resources)
    {
        res->release();
    }
}

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::reinitResources() const
{
    for (const RefCountType<ResourceType> &res : resources)
    {
        res->reinitResources();
    }
}

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::init() const
{
    for (const RefCountType<ResourceType> &res : resources)
    {
        res->init();
    }
}

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::reset()
{
    for (RefCountType<ResourceType> &res : resources)
    {
        res.reset();
    }
    resources.clear();
    basedOnSwapchain.reset();
}

template <template <typename> typename RefCountType, typename ResourceType>
bool SwapchainBufferedResource<RefCountType<ResourceType>>::isValid() const
{
    return basedOnSwapchain && !resources.empty();
}

template <template <typename> typename RefCountType, typename ResourceType>
ResourceType *SwapchainBufferedResource<RefCountType<ResourceType>>::operator->() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()].get();
}

template <template <typename> typename RefCountType, typename ResourceType>
ResourceType *SwapchainBufferedResource<RefCountType<ResourceType>>::operator*() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()].get();
}

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::set(const RefCountType<ResourceType> &resource, uint32 atIdx)
{
    debugAssert(resources.size() > atIdx);
    resources[atIdx] = resource;
}

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::setNewSwapchain(WindowCanvasRef swapchainCanvas)
{
    if (basedOnSwapchain != swapchainCanvas)
    {
        basedOnSwapchain = swapchainCanvas;
        swapchainChanged();
    }
}

template <template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::swapchainChanged()
{
    // Release and reset all extra
    for (int32 i = basedOnSwapchain->imagesCount(); i < resources.size(); ++i)
    {
        resources[i].reset();
    }
    resources.resize(basedOnSwapchain->imagesCount());
}

template <template <typename> typename RefCountType, typename ResourceType>
SwapchainBufferedResource<RefCountType<ResourceType>>::SwapchainBufferedResource(WindowCanvasRef swapchainCanvas)
    : basedOnSwapchain(swapchainCanvas)
{
    swapchainChanged();
}

//////////////////////////////////////////////////////////////////////////
/// Normal template
//////////////////////////////////////////////////////////////////////////

template <typename ResourceType>
void SwapchainBufferedResource<ResourceType>::release() const
{
    for (ResourceType *res : resources)
    {
        res->release();
    }
}

template <typename ResourceType>
void SwapchainBufferedResource<ResourceType>::reinitResources() const
{
    for (ResourceType *res : resources)
    {
        res->reinitResources();
    }
}

template <typename ResourceType>
void SwapchainBufferedResource<ResourceType>::init() const
{
    for (ResourceType *res : resources)
    {
        res->init();
    }
}

template <typename ResourceType>
void SwapchainBufferedResource<ResourceType>::reset()
{
    for (ResourceType *res : resources)
    {
        res->release();
        delete res;
    }
    resources.clear();
    basedOnSwapchain.reset();
}

template <typename ResourceType>
bool SwapchainBufferedResource<ResourceType>::isValid() const
{
    return basedOnSwapchain && !resources.empty();
}

template <typename ResourceType>
ResourceType *SwapchainBufferedResource<ResourceType>::operator->() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()];
}

template <typename ResourceType>
ResourceType *SwapchainBufferedResource<ResourceType>::operator*() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()];
}

template <typename ResourceType>
template <typename... ConstructParamTypes>
void SwapchainBufferedResource<ResourceType>::setNewSwapchain(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams)
{
    if (basedOnSwapchain != swapchainCanvas)
    {
        basedOnSwapchain = swapchainCanvas;
        swapchainChanged<ConstructParamTypes...>(std::forward<ConstructParamTypes>(constructParams)...);
    }
}

template <typename ResourceType>
template <typename... ConstructParamTypes>
void SwapchainBufferedResource<ResourceType>::swapchainChanged(ConstructParamTypes... constructParams)
{
    int32 imagesCount = basedOnSwapchain->imagesCount();
    // Release current resources for init ready and delete extra resources
    for (int32 i = 0; i < resources.size(); ++i)
    {
        resources[i]->release();
        if (imagesCount <= i)
            delete resources[i];
    }
    int32 currentResCount = int32(resources.size());
    resources.resize(imagesCount);
    // Create for newly needed resources
    for (int32 i = currentResCount; i < resources.size(); ++i)
    {
        resources[i] = new ResourceType(std::forward<ConstructParamTypes>(constructParams)...);
    }
}

template <typename ResourceType>
template <typename... ConstructParamTypes>
SwapchainBufferedResource<ResourceType>::SwapchainBufferedResource(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams)
    : basedOnSwapchain(swapchainCanvas)
{
    swapchainChanged<ConstructParamTypes...>(std::forward<ConstructParamTypes>(constructParams)...);
}
