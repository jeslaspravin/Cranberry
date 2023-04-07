/*!
 * \file GenericWindowCanvas.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <atomic>

#include "GraphicsResources.h"
#include "Math/CoreMathTypedefs.h"
#include "Memory/SmartPointers.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "Types/Containers/ReferenceCountPtr.h"
#include "Types/CoreTypes.h"

class GenericAppWindow;
class GraphicsSemaphore;
class GraphicsTimelineSemaphore;
class GraphicsFence;

using SemaphoreRef = ReferenceCountPtr<GraphicsSemaphore>;
using TimelineSemaphoreRef = ReferenceCountPtr<GraphicsTimelineSemaphore>;
using FenceRef = ReferenceCountPtr<GraphicsFence>;

// Wrapper for VkSurface and VkSwapchainKHR
class ENGINERENDERER_EXPORT GenericWindowCanvas : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GenericWindowCanvas, , GraphicsResource, )
private:
    std::atomic<uint32> refCounter;

protected:
    GenericAppWindow *ownerWindow = nullptr;

    uint32 currentSwapchainIdx = 0;
    UInt2 currentImageSize;

    GenericWindowCanvas() = default;

public:
    GenericWindowCanvas(GenericAppWindow *window)
        : ownerWindow(window)
    {}

    String getResourceName() const override;

    virtual uint32 requestNextImage(SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence = nullptr);
    uint32 currentImgIdx() const { return currentSwapchainIdx; }
    UInt2 imageSize() const { return currentImageSize; }
    virtual EPixelDataFormat::Type windowCanvasFormat() const { return EPixelDataFormat::Undefined; }
    virtual int32 imagesCount() const { return -1; }

    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* Impl ends */
};
using WindowCanvasRef = ReferenceCountPtr<GenericWindowCanvas>;
