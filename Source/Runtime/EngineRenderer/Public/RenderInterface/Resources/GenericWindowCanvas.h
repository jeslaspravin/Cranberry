#pragma once
#include <atomic>

#include "GraphicsResources.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "Memory/SmartPointers.h"
#include "Types/CoreTypes.h"
#include "Math/CoreMathTypedefs.h"
#include "Types/Containers/ReferenceCountPtr.h"

class GenericAppWindow;
class GraphicsSemaphore;
class GraphicsFence;

using SemaphoreRef = ReferenceCountPtr<GraphicsSemaphore>;
using FenceRef = ReferenceCountPtr<GraphicsFence>;

// Wrapper for VkSurface and related kind of objects
class ENGINERENDERER_EXPORT GenericWindowCanvas : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GenericWindowCanvas,,GraphicsResource,)
private:
    std::atomic<uint32> refCounter;
protected:
    GenericAppWindow* ownerWindow = nullptr;

    uint32 currentSwapchainIdx = 0;
    Size2D currentImageSize;

    GenericWindowCanvas() = default;
public:
    GenericWindowCanvas(GenericAppWindow* window)
        : ownerWindow(window)
    {}

    String getResourceName() const override;

    virtual uint32 requestNextImage(SemaphoreRef* waitOnSemaphore, FenceRef* waitOnFence = nullptr);
    uint32 currentImgIdx() const { return currentSwapchainIdx; }
    Size2D imageSize() const { return currentImageSize; }
    virtual EPixelDataFormat::Type windowCanvasFormat() const { return EPixelDataFormat::Undefined; }
    virtual int32 imagesCount() const { return -1; }

    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* Impl ends */
};
using WindowCanvasRef = ReferenceCountPtr<GenericWindowCanvas>;

