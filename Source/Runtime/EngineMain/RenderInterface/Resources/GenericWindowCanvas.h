#pragma once
#include "GraphicsResources.h"
#include "../CoreGraphicsTypes.h"
#include "../../Core/Memory/SmartPointers.h"
#include "../../Core/Platform/PlatformTypes.h"

class GenericAppWindow;
class GraphicsSemaphore;
class GraphicsFence;

// Wrapper for VkSurface and related kind of objects
class GenericWindowCanvas : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GenericWindowCanvas,,GraphicsResource,)

protected:
    GenericAppWindow* ownerWindow = nullptr;

    uint32 currentSwapchainIdx = 0;
public:
    String getResourceName() const override;

    // Setup function must be use before calling init
    virtual void setWindow(GenericAppWindow* forWindow);
    virtual uint32 requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore, SharedPtr<GraphicsFence>* waitOnFence = nullptr);
    uint32 currentImgIdx() const { return currentSwapchainIdx; }
    virtual EPixelDataFormat::Type windowCanvasFormat() const { return EPixelDataFormat::Undefined; }
    virtual int32 imagesCount() const { return -1; }
};

