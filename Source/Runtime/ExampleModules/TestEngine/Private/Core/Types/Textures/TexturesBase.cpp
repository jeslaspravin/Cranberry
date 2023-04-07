/*!
 * \file TexturesBase.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "TexturesBase.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/Resources/MemoryResources.h"

void TextureBase::setFilteringMode(ESamplerFiltering::Type filtering) { sampleFiltering = filtering; }

void TextureBase::reinitResources()
{
    textureResource->setImageSize(textureSize);
    textureResource->setNumOfMips(mipCount);
    textureResource->setSampleCounts(sampleCount);
    textureResource->setResourceName(textureName);
}

void TextureBase::setSampleCount(EPixelSampleCount::Type newSampleCount)
{
    sampleCount = newSampleCount;
    markResourceDirty();
}

void TextureBase::markResourceDirty()
{
    if (!bNeedsUpdate && textureResource.isValid())
    {
        bNeedsUpdate = true;
        ENQUEUE_RENDER_COMMAND(UpdateTexture)
        (
            [this](
                class IRenderCommandList * /*cmdList*/, IGraphicsInstance * /*graphicsInstance*/, const GraphicsHelperAPI * /*graphicsHelper*/
            )
            {
                reinitResources();
                bNeedsUpdate = false;
            }
        );
    }
}
