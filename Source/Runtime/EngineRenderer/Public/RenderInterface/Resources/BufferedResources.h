#pragma once
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "Types/Platform/PlatformAssertionErrors.h"

template<typename ResourceType>
class SwapchainBufferedResource
{
private:
    WindowCanvasRef basedOnSwapchain;
    std::vector<ResourceType*> resources;

private:
    template<typename... ConstructParamTypes>
    void swapchainChanged(ConstructParamTypes... constructParams);
public:
    SwapchainBufferedResource() = default;

    template<typename... ConstructParamTypes>
    SwapchainBufferedResource(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams);

    template<typename... ConstructParamTypes>
    void setNewSwapchain(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams);
    const std::vector<ResourceType*>& getResources() const { return resources; }
    ResourceType* operator->() const;
    ResourceType* operator*() const;
    // resets and deletes all resources
    void reset();
    bool isValid() const;

    void init() const;
    void reinitResources() const;
    void release() const;
};

template<template <typename> typename RefCountType, typename ResourceType>
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
    const std::vector<RefCountType<ResourceType>>& getResources() const { return resources; }
    ResourceType* operator->() const;
    ResourceType* operator*() const;
    void set(const RefCountType<ResourceType>& resource, uint32 atIdx);
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

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::release() const
{
    for (const RefCountType<ResourceType>& res : resources)
    {
        res->release();
    }
}

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::reinitResources() const
{
    for (const RefCountType<ResourceType>& res : resources)
    {
        res->reinitResources();
    }
}

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::init() const
{
    for (const RefCountType<ResourceType>& res : resources)
    {
        res->init();
    }
}

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::reset()
{
    for (RefCountType<ResourceType>& res : resources)
    {
        res->release();
        res.reset();
    }
    resources.clear();
}

template<template <typename> typename RefCountType, typename ResourceType>
bool SwapchainBufferedResource<RefCountType<ResourceType>>::isValid() const
{
    return basedOnSwapchain && !resources.empty();
}

template<template <typename> typename RefCountType, typename ResourceType>
ResourceType* SwapchainBufferedResource<RefCountType<ResourceType>>::operator->() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()].get();
}

template<template <typename> typename RefCountType, typename ResourceType>
ResourceType* SwapchainBufferedResource<RefCountType<ResourceType>>::operator*() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()].get();
}

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::set(const RefCountType<ResourceType>& resource, uint32 atIdx)
{
    debugAssert(resources.size() > atIdx);
    resources[atIdx] = resource;
}

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::setNewSwapchain(WindowCanvasRef swapchainCanvas)
{
    if (basedOnSwapchain != swapchainCanvas)
    {
        basedOnSwapchain = swapchainCanvas;
        swapchainChanged();
    }
}

template<template <typename> typename RefCountType, typename ResourceType>
void SwapchainBufferedResource<RefCountType<ResourceType>>::swapchainChanged()
{
    // Release and reset all extra
    for (int32 i = basedOnSwapchain->imagesCount(); i < resources.size(); ++i)
    {
        resources[i]->release();
        resources[i].reset();
    }
    resources.resize(basedOnSwapchain->imagesCount());
}

template<template <typename> typename RefCountType, typename ResourceType>
SwapchainBufferedResource<RefCountType<ResourceType>>::SwapchainBufferedResource(WindowCanvasRef swapchainCanvas)
    : basedOnSwapchain(swapchainCanvas)
{
    swapchainChanged();
}

//////////////////////////////////////////////////////////////////////////
/// Normal template
//////////////////////////////////////////////////////////////////////////

template<typename ResourceType>
void SwapchainBufferedResource<ResourceType>::release() const
{
    for (ResourceType* res : resources)
    {
        res->release();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<ResourceType>::reinitResources() const
{
    for (ResourceType* res : resources)
    {
        res->reinitResources();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<ResourceType>::init() const
{
    for (ResourceType* res : resources)
    {
        res->init();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<ResourceType>::reset()
{
    for (ResourceType* res : resources)
    {
        res->release();
        delete res;
    }
    resources.clear();
}

template<typename ResourceType>
bool SwapchainBufferedResource<ResourceType>::isValid() const
{
    return basedOnSwapchain && !resources.empty();
}

template<typename ResourceType>
ResourceType* SwapchainBufferedResource<ResourceType>::operator->() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()];
}

template<typename ResourceType>
ResourceType* SwapchainBufferedResource<ResourceType>::operator*() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()];
}

template<typename ResourceType>
template<typename... ConstructParamTypes>
void SwapchainBufferedResource<ResourceType>::setNewSwapchain(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams)
{
    if (basedOnSwapchain != swapchainCanvas)
    {
        basedOnSwapchain = swapchainCanvas;
        swapchainChanged<ConstructParamTypes...>(std::forward<ConstructParamTypes>(constructParams)...);
    }
}

template<typename ResourceType>
template<typename... ConstructParamTypes>
void SwapchainBufferedResource<ResourceType>::swapchainChanged(ConstructParamTypes... constructParams)
{
    int32 imagesCount = basedOnSwapchain->imagesCount();
    // Release current resources for init ready and delete extra resources
    for (int32 i = 0; i < resources.size(); ++i)
    {
        resources[i]->release();
        if(imagesCount <= i)
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

template<typename ResourceType>
template<typename... ConstructParamTypes>
SwapchainBufferedResource<ResourceType>::SwapchainBufferedResource(WindowCanvasRef swapchainCanvas, ConstructParamTypes... constructParams)
    : basedOnSwapchain(swapchainCanvas)
{
    swapchainChanged<ConstructParamTypes...>(std::forward<ConstructParamTypes>(constructParams)...);
}
