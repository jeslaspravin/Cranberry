/*!
 * \file MemoryResources.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/MemoryResources.h"
#include "Math/Math.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"

DEFINE_GRAPHICS_RESOURCE(MemoryResource)

void MemoryResource::addRef()
{
    uint32 count;
    count = refCounter.fetch_add(1, std::memory_order::release);
}

void MemoryResource::removeRef()
{
    uint32 count = refCounter.fetch_sub(1, std::memory_order::acq_rel);
    if (count == 1)
    {
        ENQUEUE_RENDER_COMMAND(DeleteMemoryResource)
        (
            [this](class IRenderCommandList * /*cmdList*/, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                graphicsHelper->markForDeletion(
                    graphicsInstance, this, bDeferDelete ? EDeferredDelStrategy::SwapchainCount : EDeferredDelStrategy::Immediate
                );
            }
        );
    }
}

uint32 MemoryResource::refCount() const { return refCounter.load(std::memory_order::acquire); }

String MemoryResource::getResourceName() const { return memoryResName; }

void MemoryResource::setResourceName(const String &name) { memoryResName = name; }

DEFINE_GRAPHICS_RESOURCE(BufferResource)
DEFINE_GRAPHICS_RESOURCE(ImageResource)

ImageResource::ImageResource(ImageResourceCreateInfo createInfo)
    : MemoryResource(createInfo.imageFormat)
    , dimensions(createInfo.dimensions)
    , numOfMips(createInfo.numOfMips)
    , layerCount(createInfo.layerCount)
{}

uint32 ImageResource::mipCountFromDim()
{
    return (uint32)(1 + Math::floor(Math::log2((float)Math::max(dimensions.x, dimensions.y, dimensions.z))));
}

void ImageResource::setLayerCount(uint32 count) { layerCount = Math::max<uint32>(count, 1); }

void ImageResource::setSampleCounts(EPixelSampleCount::Type samples) { sampleCounts = samples; }

void ImageResource::setNumOfMips(uint32 mipCount) { numOfMips = mipCount; }

void ImageResource::setShaderUsage(uint32 usage) { shaderUsage = usage; }

void ImageResource::setImageSize(const UInt3 &imageSize) { dimensions = imageSize; }