/*!
 * \file MemoryResources.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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
        ENQUEUE_COMMAND(DeleteMemoryResource)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
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

void ImageResource::setImageSize(const Size3D &imageSize) { dimensions = imageSize; }

bool EPixelDataFormat::isDepthFormat(Type dataFormat)
{
    return EPixelDataFormat::DepthFormatBegin <= dataFormat && EPixelDataFormat::DepthFormatEnd >= dataFormat;
}

bool EPixelDataFormat::isStencilFormat(Type dataFormat)
{
    return EPixelDataFormat::StencilDepthEnd <= dataFormat && EPixelDataFormat::StencilDepthEnd >= dataFormat;
}

bool EPixelDataFormat::isPureIntegralFormat(EPixelDataFormat::Type dataFormat)
{
    return EPixelDataFormat::IntFormatBegin <= dataFormat && EPixelDataFormat::IntFormatEnd >= dataFormat;
}
bool EPixelDataFormat::isFloatingFormat(Type dataFormat)
{
    return EPixelDataFormat::FloatFormatBegin <= dataFormat && EPixelDataFormat::FloatFormatEnd >= dataFormat;
}
bool EPixelDataFormat::isNormalizedFormat(EPixelDataFormat::Type dataFormat)
{
    return EPixelDataFormat::NormFormatBegin <= dataFormat && EPixelDataFormat::NormFormatEnd >= dataFormat;
}
bool EPixelDataFormat::isScaledFormat(EPixelDataFormat::Type dataFormat)
{
    return EPixelDataFormat::ScaledFormatEnd <= dataFormat && EPixelDataFormat::ScaledFormatBegin >= dataFormat;
}

bool EPixelDataFormat::isSignedFormat(Type dataFormat)
{
    return isFloatingFormat(dataFormat) || (EPixelDataFormat::SIntFormatBegin <= dataFormat && EPixelDataFormat::SIntFormatEnd >= dataFormat)
           || (EPixelDataFormat::SNormFormatBegin <= dataFormat && EPixelDataFormat::SNormFormatEnd >= dataFormat)
           || (EPixelDataFormat::SScaledFormatBegin <= dataFormat && EPixelDataFormat::SScaledFormatEnd >= dataFormat);
}
bool EPixelDataFormat::isUnsignedFormat(Type dataFormat)
{
    return (EPixelDataFormat::UIntFormatBegin <= dataFormat && EPixelDataFormat::UIntFormatEnd >= dataFormat)
           || (EPixelDataFormat::UNormFormatBegin <= dataFormat && EPixelDataFormat::UNormFormatEnd >= dataFormat)
           || (EPixelDataFormat::UScaledFormatBegin <= dataFormat && EPixelDataFormat::UScaledFormatEnd >= dataFormat);
}
bool EPixelDataFormat::isSrgbFormat(EPixelDataFormat::Type dataFormat)
{
    return (EPixelDataFormat::SRGBFormatBegin <= dataFormat && EPixelDataFormat::SRGBFormatEnd >= dataFormat);
}