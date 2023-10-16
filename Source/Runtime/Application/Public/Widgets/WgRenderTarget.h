/*!
 * \file WgRenderTarget.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "Math/CoreMathTypedefs.h"
#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/MemoryResources.h"

struct WgRenderTargetCI
{
    String textureName;
    Short2 textureSize;
    EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1;
    bool bIsSrgb = false;
};

class APPLICATION_EXPORT WgRenderTarget : public IRenderTargetTexture
{
private:
    ImageResourceRef rtTexture;
    ImageResourceRef resolvedTexture;

public:
    // Had to manually default every copy and move as there is custom destructor
    WgRenderTarget() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(WgRenderTarget)

    ~WgRenderTarget() { destroy(); }

    void init(WgRenderTargetCI createInfo);
    void destroy();

    MemoryResourceRef renderResource() const override { return resolvedTexture; }
    MemoryResourceRef renderTargetResource() const override { return rtTexture; }
};