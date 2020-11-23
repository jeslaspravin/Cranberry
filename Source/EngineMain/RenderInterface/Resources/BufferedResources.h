#pragma once
#include "GenericWindowCanvas.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

template<typename ResourceType>
class SwapchainBufferedResource
{
private:
    GenericWindowCanvas* basedOnSwapchain;
    std::vector<ResourceType*> resources;

private:
    template<typename... ConstructParamTypes>
    void swapchainChanged(ConstructParamTypes... constructParams);
public:
    SwapchainBufferedResource() = default;

    template<typename... ConstructParamTypes>
    SwapchainBufferedResource(GenericWindowCanvas* swapchainCanvas, ConstructParamTypes... constructParams);

    template<typename... ConstructParamTypes>
    void setNewSwapchain(GenericWindowCanvas* swapchainCanvas, ConstructParamTypes... constructParams);
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

template<typename ResourceType>
class SwapchainBufferedResource<SharedPtr<ResourceType>>
{
private:
    GenericWindowCanvas* basedOnSwapchain;
    std::vector<SharedPtr<ResourceType>> resources;

private:
    void swapchainChanged();
public:
    SwapchainBufferedResource() = default;
    SwapchainBufferedResource(GenericWindowCanvas* swapchainCanvas);

    void setNewSwapchain(GenericWindowCanvas* swapchainCanvas);
    const std::vector<SharedPtr<ResourceType>>& getResources() const { return resources; }
    ResourceType* operator->() const;
    ResourceType* operator*() const;
    void set(const SharedPtr<ResourceType>& resource, uint32 atIdx);
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

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::release() const
{
    for (const SharedPtr<ResourceType>& res : resources)
    {
        res->release();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::reinitResources() const
{
    for (const SharedPtr<ResourceType>& res : resources)
    {
        res->reinitResources();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::init() const
{
    for (const SharedPtr<ResourceType>& res : resources)
    {
        res->init();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::reset()
{
    for (SharedPtr<ResourceType>& res : resources)
    {
        res->release();
        res.reset();
    }
    resources.clear();
}

template<typename ResourceType>
bool SwapchainBufferedResource< SharedPtr<ResourceType>>::isValid() const
{
    return basedOnSwapchain && !resources.empty();
}

template<typename ResourceType>
ResourceType* SwapchainBufferedResource<SharedPtr<ResourceType>>::operator->() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()].get();
}

template<typename ResourceType>
ResourceType* SwapchainBufferedResource<SharedPtr<ResourceType>>::operator*() const
{
    debugAssert(resources.size() == basedOnSwapchain->imagesCount());
    return resources[basedOnSwapchain->currentImgIdx()].get();
}

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::set(const SharedPtr<ResourceType>& resource, uint32 atIdx)
{
    debugAssert(resources.size() > atIdx);
    resources[atIdx] = resource;
}

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::setNewSwapchain(GenericWindowCanvas* swapchainCanvas)
{
    if (basedOnSwapchain != swapchainCanvas)
    {
        basedOnSwapchain = swapchainCanvas;
        swapchainChanged();
    }
}

template<typename ResourceType>
void SwapchainBufferedResource<SharedPtr<ResourceType>>::swapchainChanged()
{
    // Release and reset all extra
    for (int32 i = basedOnSwapchain->imagesCount(); i < resources.size(); ++i)
    {
        resources[i]->release();
        resources[i].reset();
    }
    resources.resize(basedOnSwapchain->imagesCount());
}

template<typename ResourceType>
SwapchainBufferedResource<SharedPtr<ResourceType>>::SwapchainBufferedResource(GenericWindowCanvas* swapchainCanvas)
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
void SwapchainBufferedResource<ResourceType>::setNewSwapchain(GenericWindowCanvas* swapchainCanvas, ConstructParamTypes... constructParams)
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
SwapchainBufferedResource<ResourceType>::SwapchainBufferedResource(GenericWindowCanvas* swapchainCanvas, ConstructParamTypes... constructParams)
    : basedOnSwapchain(swapchainCanvas)
{
    swapchainChanged<ConstructParamTypes...>(std::forward<ConstructParamTypes>(constructParams)...);
}
