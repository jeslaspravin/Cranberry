/*!
 * \file GraphicsHelper.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <set>
#include <vector> // TODO(Jeslas) : change vector ptrs to const ref vectors

#include "EngineRendererExports.h"
#include "Types/CoreTypes.h"
#include "Memory/SmartPointers.h"
#include "RenderInterface/Resources/DeferredDeleter.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
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

/**
 * Helper functions that are related to general helper functions and are stateless. Using instance to allow virtual calls to API specific
 * functions
 */
class ENGINERENDERER_EXPORT GraphicsHelperAPI
{
public:
    virtual SemaphoreRef createSemaphore(class IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const = 0;
    virtual TimelineSemaphoreRef createTimelineSemaphore(class IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const = 0;
    virtual void waitTimelineSemaphores(
        class IGraphicsInstance *graphicsInstance, std::vector<TimelineSemaphoreRef> *semaphores, std::vector<uint64> *waitForValues
    ) const = 0;

    virtual FenceRef createFence(class IGraphicsInstance *graphicsInstance, const TChar *fenceName, bool bIsSignaled = false) const = 0;
    virtual void waitFences(class IGraphicsInstance *graphicsInstance, std::vector<FenceRef> *fences, bool waitAll) const = 0;

    virtual SamplerRef createSampler(class IGraphicsInstance *graphicsInstance, SamplerCreateInfo createInfo) const = 0;

    virtual ESamplerFiltering::Type clampFiltering(
        class IGraphicsInstance *graphicsInstance, ESamplerFiltering::Type sampleFiltering, EPixelDataFormat::Type imageFormat
    ) const = 0;

    virtual WindowCanvasRef createWindowCanvas(class IGraphicsInstance *graphicsInstance, GenericAppWindow *fromWindow) const = 0;
    virtual void cacheSurfaceProperties(class IGraphicsInstance *graphicsInstance, const WindowCanvasRef &windowCanvas) const = 0;

    // Normal data buffers
    virtual BufferResourceRef
        createReadOnlyBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;
    // Cannot be used as uniform
    virtual BufferResourceRef
        createWriteOnlyBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;
    // Can be used as both uniform and storage
    virtual BufferResourceRef
        createReadWriteBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;

    // Texels buffers
    virtual BufferResourceRef
        createReadOnlyTexels(class IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1) const = 0;
    // Cannot be used as uniform sampled
    virtual BufferResourceRef
        createWriteOnlyTexels(class IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1) const = 0;
    virtual BufferResourceRef
        createReadWriteTexels(class IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1) const = 0;

    // Other utility buffers
    virtual BufferResourceRef
        createReadOnlyIndexBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;
    virtual BufferResourceRef
        createReadOnlyVertexBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;

    virtual BufferResourceRef
        createReadOnlyIndirectBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;
    virtual BufferResourceRef
        createWriteOnlyIndirectBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const = 0;

    // Images, Images are created with staging as we are not going to use image and staging and this is
    // for advanced use only
    virtual ImageResourceRef
        createImage(class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const = 0;
    virtual ImageResourceRef
        createCubeImage(class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const = 0;
    virtual ImageResourceRef createRTImage(
        class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo,
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1
    ) const = 0;
    virtual ImageResourceRef createCubeRTImage(
        class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo,
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1
    ) const = 0;

    virtual void mapResource(class IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const = 0;
    virtual void unmapResource(class IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const = 0;
    virtual void mapResource(class IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const = 0;
    virtual void unmapResource(class IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const = 0;
    virtual void *borrowMappedPtr(class IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const = 0;
    virtual void returnMappedPtr(class IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const = 0;
    virtual void flushMappedPtr(class IGraphicsInstance *graphicsInstance, const std::vector<ImageResourceRef> &resources) const = 0;
    virtual void *borrowMappedPtr(class IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const = 0;
    virtual void returnMappedPtr(class IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const = 0;
    virtual void flushMappedPtr(class IGraphicsInstance *graphicsInstance, const std::vector<BufferResourceRef> &resources) const = 0;

    // Marks this resource for deletion if deferred delete is present else deletes it immediately, delete
    // strategy will be used if deferring is allowed duration determines time duration/number of frames
    // based on the strategy
    virtual void markForDeletion(
        class IGraphicsInstance *graphicsInstance, GraphicsResource *resource, EDeferredDelStrategy deleteStrategy, TickRep duration = 1
    ) const = 0;
    virtual void markForDeletion(
        class IGraphicsInstance *graphicsInstance, SimpleSingleCastDelegate deleter, EDeferredDelStrategy deleteStrategy, TickRep duration = 1
    ) const = 0;

    // Pipelines
    virtual PipelineBase *createGraphicsPipeline(class IGraphicsInstance *graphicsInstance, const PipelineBase *parent) const = 0;
    virtual PipelineBase *createGraphicsPipeline(class IGraphicsInstance *graphicsInstance, const GraphicsPipelineConfig &config) const = 0;

    virtual PipelineBase *createComputePipeline(class IGraphicsInstance *graphicsInstance, const PipelineBase *parent) const = 0;
    virtual PipelineBase *createComputePipeline(class IGraphicsInstance *graphicsInstance) const = 0;

    // Application specific
    virtual GlobalRenderingContextBase *createGlobalRenderingContext() const = 0;
    virtual ShaderResource *createShaderResource(const ShaderConfigCollector *inConfig) const = 0;
    virtual ShaderParametersRef createShaderParameters(
        class IGraphicsInstance *graphicsInstance, const class GraphicsResource *paramLayout, const std::set<uint32> &ignoredSetIds = {}
    ) const = 0;

    virtual Framebuffer *createFbInstance() const = 0;
    virtual void initializeFb(class IGraphicsInstance *graphicsInstance, Framebuffer *fb, const Size2D &frameSize) const = 0;
    virtual void initializeSwapchainFb(class IGraphicsInstance *graphicsInstance, Framebuffer *fb, WindowCanvasRef canvas, uint32 swapchainIdx)
        const = 0;

    // All write buffers are storage buffers, Read buffers means that can be used as Uniform
    virtual const GraphicsResourceType *readOnlyBufferType() const = 0;
    virtual const GraphicsResourceType *writeOnlyBufferType() const = 0;
    virtual const GraphicsResourceType *readWriteBufferType() const = 0;
    virtual const GraphicsResourceType *readOnlyTexelsType() const = 0;
    virtual const GraphicsResourceType *writeOnlyTexelsType() const = 0;
    virtual const GraphicsResourceType *readWriteTexelsType() const = 0;
    virtual const GraphicsResourceType *readOnlyIndexBufferType() const = 0;
    virtual const GraphicsResourceType *readOnlyVertexBufferType() const = 0;
    virtual const GraphicsResourceType *readOnlyIndirectBufferType() const = 0;
    virtual const GraphicsResourceType *writeOnlyIndirectBufferType() const = 0;
    FORCE_INLINE bool isReadOnlyBuffer(BufferResourceRef resource) const
    {
        return resource->getType()->isChildOf(readOnlyBufferType()) || resource->getType()->isChildOf(readOnlyTexelsType())
               || resource->getType()->isChildOf(readOnlyIndexBufferType()) || resource->getType()->isChildOf(readOnlyVertexBufferType())
               || resource->getType()->isChildOf(readOnlyIndirectBufferType());
    }
    FORCE_INLINE bool isWriteOnlyBuffer(BufferResourceRef resource) const
    {
        return resource->getType()->isChildOf(writeOnlyBufferType()) || resource->getType()->isChildOf(writeOnlyTexelsType())
               || resource->getType()->isChildOf(writeOnlyIndirectBufferType());
    }
    FORCE_INLINE bool isRWBuffer(BufferResourceRef resource) const
    {
        return resource->getType()->isChildOf(readWriteBufferType()) || resource->getType()->isChildOf(readWriteTexelsType());
    }
    FORCE_INLINE bool isTexelBuffer(BufferResourceRef resource) const
    {
        return resource->getType()->isChildOf(readOnlyTexelsType()) || resource->getType()->isChildOf(writeOnlyTexelsType())
               || resource->getType()->isChildOf(readWriteTexelsType());
    }

    virtual const GraphicsResourceType *imageType() const = 0;
    virtual const GraphicsResourceType *cubeImageType() const = 0;
    virtual const GraphicsResourceType *rtImageType() const = 0;
    virtual const GraphicsResourceType *cubeRTImageType() const = 0;
};