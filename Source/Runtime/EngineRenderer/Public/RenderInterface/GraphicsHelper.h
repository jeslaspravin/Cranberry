#pragma once
#include <vector>// TODO(Jeslas) : change vector ptrs to const ref vectors
#include <set>

#include "Memory/SmartPointers.h"
#include "Types/CoreTypes.h"
#include "EngineRendererExports.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"


class GlobalRenderingContextBase;
class ShaderConfigCollector;
struct Framebuffer;
struct GraphicsPipelineConfig;
class PipelineBase;

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

class ENGINERENDERER_EXPORT GraphicsHelperAPI
{
public:

    virtual SemaphoreRef createSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName) const = 0;
    virtual TimelineSemaphoreRef createTimelineSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName) const = 0;
    virtual void waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
        std::vector<TimelineSemaphoreRef>* semaphores, std::vector<uint64>* waitForValues) const = 0;

    virtual FenceRef createFence(class IGraphicsInstance* graphicsInstance,const char* fenceName, bool bIsSignaled = false) const = 0;
    virtual void waitFences(class IGraphicsInstance* graphicsInstance, std::vector<FenceRef>* fences, bool waitAll) const = 0;

    virtual SamplerRef createSampler(class IGraphicsInstance* graphicsInstance, SamplerCreateInfo createInfo) const = 0;

    virtual ESamplerFiltering::Type clampFiltering(class IGraphicsInstance* graphicsInstance, ESamplerFiltering::Type sampleFiltering
        , EPixelDataFormat::Type imageFormat) const = 0;

    virtual WindowCanvasRef createWindowCanvas(class IGraphicsInstance* graphicsInstance, GenericAppWindow* fromWindow) const = 0;
    virtual void cacheSurfaceProperties(class IGraphicsInstance* graphicsInstance, const WindowCanvasRef& windowCanvas) const = 0;

    // Normal data buffers
    virtual BufferResourceRef createReadOnlyBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;
    // Cannot be used as uniform
    virtual BufferResourceRef createWriteOnlyBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;
    // Can be used as both uniform and storage
    virtual BufferResourceRef createReadWriteBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;

    // Texels buffers
    virtual BufferResourceRef createReadOnlyTexels(class IGraphicsInstance* graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;
    // Cannot be used as uniform sampled
    virtual BufferResourceRef createWriteOnlyTexels(class IGraphicsInstance* graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;
    virtual BufferResourceRef createReadWriteTexels(class IGraphicsInstance* graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;

    // Other utility buffers
    virtual BufferResourceRef createReadOnlyIndexBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;
    virtual BufferResourceRef createReadOnlyVertexBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;

    virtual BufferResourceRef createReadOnlyIndirectBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;
    virtual BufferResourceRef createWriteOnlyIndirectBuffer(class IGraphicsInstance* graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1, bool bIsStaging = false) const = 0;

    // Images
    virtual ImageResourceRef createImage(class IGraphicsInstance* graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const = 0;
    virtual ImageResourceRef createCubeImage(class IGraphicsInstance* graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const = 0;
    virtual ImageResourceRef createRTImage(class IGraphicsInstance* graphicsInstance, ImageResourceCreateInfo createInfo, EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1) const = 0;
    virtual ImageResourceRef createCubeRTImage(class IGraphicsInstance* graphicsInstance, ImageResourceCreateInfo createInfo, EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1) const = 0;

    virtual void mapResource(class IGraphicsInstance* graphicsInstance, BufferResourceRef& buffer) const = 0;
    virtual void unmapResource(class IGraphicsInstance* graphicsInstance, BufferResourceRef& buffer) const = 0;
    virtual void mapResource(class IGraphicsInstance* graphicsInstance, ImageResourceRef& image) const = 0;
    virtual void unmapResource(class IGraphicsInstance* graphicsInstance, ImageResourceRef& image) const = 0;
    virtual void* borrowMappedPtr(class IGraphicsInstance* graphicsInstance, ImageResourceRef& resource) const = 0;
    virtual void returnMappedPtr(class IGraphicsInstance* graphicsInstance, ImageResourceRef& resource) const = 0;
    virtual void flushMappedPtr(class IGraphicsInstance* graphicsInstance, const std::vector<ImageResourceRef>& resources) const = 0;
    virtual void* borrowMappedPtr(class IGraphicsInstance* graphicsInstance, BufferResourceRef& resource) const = 0;
    virtual void returnMappedPtr(class IGraphicsInstance* graphicsInstance, BufferResourceRef& resource) const = 0;
    virtual void flushMappedPtr(class IGraphicsInstance* graphicsInstance, const std::vector<BufferResourceRef>& resources) const = 0;

    // Pipelines
    virtual PipelineBase* createGraphicsPipeline(class IGraphicsInstance* graphicsInstance, const PipelineBase* parent) const = 0;
    virtual PipelineBase* createGraphicsPipeline(class IGraphicsInstance* graphicsInstance, const GraphicsPipelineConfig& config) const = 0;
                        
    virtual PipelineBase* createComputePipeline(class IGraphicsInstance* graphicsInstance, const PipelineBase* parent) const = 0;
    virtual PipelineBase* createComputePipeline(class IGraphicsInstance* graphicsInstance) const = 0;

    // Application specific
    virtual GlobalRenderingContextBase* createGlobalRenderingContext() const = 0;
    virtual ShaderResource* createShaderResource(const ShaderConfigCollector* inConfig) const = 0;
    virtual ShaderParametersRef createShaderParameters(class IGraphicsInstance* graphicsInstance
        , const class GraphicsResource* paramLayout, const std::set<uint32>& ignoredSetIds = {}) const = 0; 

    virtual Framebuffer* createFbInstance() const = 0;
    virtual void initializeFb(class IGraphicsInstance* graphicsInstance, Framebuffer* fb, const Size2D& frameSize) const = 0;
    virtual void initializeSwapchainFb(class IGraphicsInstance* graphicsInstance, Framebuffer* fb, WindowCanvasRef canvas, uint32 swapchainIdx) const = 0;

    virtual const GraphicsResourceType* readOnlyBufferType() const = 0;
    virtual const GraphicsResourceType* writeOnlyBufferType() const = 0;
    virtual const GraphicsResourceType* readWriteBufferType() const = 0;
    virtual const GraphicsResourceType* readOnlyTexelsType() const = 0;
    virtual const GraphicsResourceType* writeOnlyTexelsType() const = 0;
    virtual const GraphicsResourceType* readWriteTexelsType() const = 0;
    virtual const GraphicsResourceType* readOnlyIndexBufferType() const = 0;
    virtual const GraphicsResourceType* readOnlyVertexBufferType() const = 0;
    virtual const GraphicsResourceType* readOnlyIndirectBufferType() const = 0;
    virtual const GraphicsResourceType* writeOnlyIndirectBufferType() const = 0;

    virtual const GraphicsResourceType* imageType() const = 0;
    virtual const GraphicsResourceType* cubeImageType() const = 0;
    virtual const GraphicsResourceType* rtImageType() const = 0;
    virtual const GraphicsResourceType* cubeRTImageType() const = 0;
};