#pragma once
#include "../Core/Memory/SmartPointers.h"
#include "../Core/Platform/PlatformTypes.h"

#include <vector>// TODO(Jeslas) : change vector ptrs to const ref vectors
#include <set>

namespace ESamplerTilingMode
{
    enum Type;
}
namespace ESamplerFiltering
{
    enum Type;
}
namespace EPixelDataFormat
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

    static void presentImage(class IGraphicsInstance* graphicsInstance, const std::vector<class GenericWindowCanvas*>* canvases,
        const std::vector<uint32>* imageIndex, const std::vector<SharedPtr<class GraphicsSemaphore>>* waitOnSemaphores)
    {
        HelperType::presentImage(graphicsInstance, canvases,imageIndex,waitOnSemaphores);
    }

    static SharedPtr<class SamplerInterface> createSampler(class IGraphicsInstance* graphicsInstance, const char* name,
        ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering, float poorMipLod = 0, uint8 samplerBorderColFlags = 0)
    {
        return HelperType::createSampler(graphicsInstance, name, samplerTiling, samplerFiltering, poorMipLod, samplerBorderColFlags);
    }

    static SharedPtr<class ShaderParameters> createShaderParameters(class IGraphicsInstance* graphicsInstance
        , const class GraphicsResource* paramLayout, const std::set<uint32>& ignoredSetIds = {})
    {
        return HelperType::createShaderParameters(graphicsInstance, paramLayout, ignoredSetIds);
    }

    static ESamplerFiltering::Type getClampedFiltering(class IGraphicsInstance* graphicsInstance, ESamplerFiltering::Type sampleFiltering
        , EPixelDataFormat::Type imageFormat)
    {
        return HelperType::getClampedFiltering(graphicsInstance, sampleFiltering, imageFormat);
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
    static void flushMappedPtr(class IGraphicsInstance* graphicsInstance, const std::vector<class GraphicsResource*>& resources)
    {
        return HelperType::flushMappedPtr(graphicsInstance, resources);
    }

    static uint32 shaderToPipelineStageFlags(uint32 shaderStageFlags)
    {
        return HelperType::shaderToPipelineStageFlags(shaderStageFlags);
    }
    static uint32 pipelineToShaderStageFlags(uint32 pipelineStageFlags)
    {
        return HelperType::pipelineToShaderStageFlags(pipelineStageFlags);
    }
};
