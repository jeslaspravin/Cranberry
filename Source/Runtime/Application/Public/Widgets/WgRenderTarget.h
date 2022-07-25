/*!
 * \file WgRenderTarget.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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
    Short2D textureSize;
    EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1;
    bool bIsSrgb = false;
};

class APPLICATION_EXPORT WgRenderTarget : public IRenderTargetTexture
{
private:
    ImageResourceRef rtTexture;
    ImageResourceRef resolvedTexture;

public:
    ~WgRenderTarget() { destroy(); }

    void init(WgRenderTargetCI createInfo);
    void destroy();

    MemoryResourceRef renderResource() const override { return resolvedTexture; }
    MemoryResourceRef renderTargetResource() const override { return rtTexture; }
};