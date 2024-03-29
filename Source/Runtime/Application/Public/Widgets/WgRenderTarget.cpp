/*!
 * \file WgRenderTarget.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgRenderTarget.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"

void WgRenderTarget::init(WgRenderTargetCI createInfo)
{
    if (createInfo.textureSize.x == 0 || createInfo.textureSize.y == 0)
    {
        destroy();
        return;
    }
    ENQUEUE_RENDER_COMMAND(WgRenderTargetInit)
    (
        [createInfo, this](class IRenderCommandList * /*cmdList*/, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            bool bInitialize = !rtTexture.isValid() || !rtTexture->isValid() || rtTexture->sampleCount() != createInfo.sampleCount
                               || rtTexture->getImageSize().x != uint32(createInfo.textureSize.x)
                               || rtTexture->getImageSize().y != uint32(createInfo.textureSize.y);
            if (createInfo.bIsSrgb)
            {
                bInitialize = bInitialize || rtTexture->imageFormat() == EPixelDataFormat::BGRA_U8_Norm;
            }
            else
            {
                bInitialize = bInitialize || rtTexture->imageFormat() == EPixelDataFormat::BGRA_U8_SRGB;
            }

            if (bInitialize)
            {
                ImageResourceCreateInfo imageCI{
                    .imageFormat = (createInfo.bIsSrgb ? EPixelDataFormat::BGRA_U8_SRGB : EPixelDataFormat::BGRA_U8_Norm),
                    .dimensions = {createInfo.textureSize.x, createInfo.textureSize.y, 1},
                    .numOfMips = 1
                };

                rtTexture = graphicsHelper->createRTImage(graphicsInstance, imageCI, createInfo.sampleCount);
                rtTexture->setShaderUsage(EImageShaderUsage::Sampling);
                rtTexture->setResourceName(createInfo.textureName + TCHAR("_RT"));
                rtTexture->init();

                if (createInfo.sampleCount == EPixelSampleCount::SampleCount1)
                {
                    resolvedTexture = rtTexture;
                }
                else
                {
                    resolvedTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
                    resolvedTexture->setShaderUsage(EImageShaderUsage::Sampling);
                    rtTexture->setResourceName(createInfo.textureName + TCHAR("_Resolve"));
                    resolvedTexture->init();
                }
            }
        }
    );
}

void WgRenderTarget::destroy()
{
    rtTexture.reset();
    resolvedTexture.reset();
}
