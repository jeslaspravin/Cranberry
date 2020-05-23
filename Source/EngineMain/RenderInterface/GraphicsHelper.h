#pragma once
#include "../Core/Memory/SmartPointers.h"
#include "../Core/Platform/PlatformTypes.h"

/* Fwd declarations */
namespace std
{
    template <class _Ty>
    class allocator;

    template <class _Ty, class _Alloc = allocator<_Ty>>
    class vector;
}
namespace ESamplerTilingMode
{
    enum Type;
}
namespace ESamplerFiltering
{
    enum Type;
}
/* Fwd declarations end */

template<typename HelperType>
class GraphicsHelperAPI
{
private:
    GraphicsHelperAPI() = default;

public:
    
    static SharedPtr<class GraphicsSemaphore> createSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName)
    {
        return HelperType::createSemaphore(graphicsInstance,semaphoreName);
    }

    static SharedPtr<class GraphicsTimelineSemaphore> createTimelineSemaphore(class IGraphicsInstance* graphicsInstance,
        const char* semaphoreName)
    {
        return HelperType::createTimelineSemaphore(graphicsInstance,semaphoreName);
    }

    static void waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
        std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores, std::vector<uint64>* waitForValues)
    {
        HelperType::waitTimelineSemaphores(graphicsInstance, semaphores,waitForValues);
    }

    static SharedPtr<class GraphicsFence> createFence(class IGraphicsInstance* graphicsInstance,const char* fenceName, bool bIsSignaled = false)
    {
        return HelperType::createFence(graphicsInstance,fenceName, bIsSignaled);
    }

    static void waitFences(class IGraphicsInstance* graphicsInstance,
        std::vector<SharedPtr<class GraphicsFence>>* fences, bool waitAll)
    {
        HelperType::waitFences(graphicsInstance, fences,waitAll);
    }

    static void presentImage(class IGraphicsInstance* graphicsInstance, std::vector<class GenericWindowCanvas*>* canvases,
        std::vector<uint32>* imageIndex, std::vector<SharedPtr<class GraphicsSemaphore>>* waitOnSemaphores)
    {
        HelperType::presentImage(graphicsInstance, canvases,imageIndex,waitOnSemaphores);
    }

    static SharedPtr<class SamplerInterface> createSampler(class IGraphicsInstance* graphicsInstance, const char* name,
        ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering, float poorMipLod = 0)
    {
        return HelperType::createSampler(graphicsInstance, name, samplerTiling, samplerFiltering, poorMipLod);
    }

    static void mapResource(class IGraphicsInstance* graphicsInstance, class BufferResource* buffer)
    {
        return HelperType::mapResource(graphicsInstance, buffer);
    }
    static void unmapResource(class IGraphicsInstance* graphicsInstance, class BufferResource* buffer)
    {
        return HelperType::unmapResource(graphicsInstance, buffer);
    }
    static void mapResource(class IGraphicsInstance* graphicsInstance, class ImageResource* image)
    {
        return HelperType::mapResource(graphicsInstance, image);
    }
    static void unmapResource(class IGraphicsInstance* graphicsInstance, class ImageResource* image)
    {
        return HelperType::unmapResource(graphicsInstance, image);
    }
    static void* borrowMappedPtr(class IGraphicsInstance* graphicsInstance, class GraphicsResource* resource)
    {
        return HelperType::borrowMappedPtr(graphicsInstance, resource);
    }
    static void returnMappedPtr(class IGraphicsInstance* graphicsInstance, class GraphicsResource* resource)
    {
        return HelperType::returnMappedPtr(graphicsInstance, resource);
    }
};
