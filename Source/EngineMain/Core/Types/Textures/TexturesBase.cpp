#include "TexturesBase.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderInterface/Resources/MemoryResources.h"

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
    if (!bNeedsUpdate)
    {
        ENQUEUE_COMMAND(UpdateTexture,
            {
                reinitResources();
                bNeedsUpdate = false;
            }
        , this);
    }
}
