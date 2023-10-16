/*!
 * \file CubeTextures.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CubeTextures.h"
#include "Math/Math.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

uint32 CubeTexture::getMipCount() const { return mipCount; }

void CubeTexture::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_RENDER_COMMAND(ReinitCubeTexture)
    (
        [this](IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
            }
            else
            {
                CubeTexture::init(this);
            }
        }
    );
}

EPixelDataFormat::Type CubeTexture::determineDataFormat(ECubeTextureFormat dataFormat)
{
    switch (dataFormat)
    {
    case ECubeTextureFormat::CT_F32:
        return EPixelDataFormat::RGBA_SF32;
        break;
    case ECubeTextureFormat::CT_F16:
        return EPixelDataFormat::RGBA_SF16;
        break;
    case ECubeTextureFormat::CT_NormalizedUI8:
        return EPixelDataFormat::RGBA_U8_Norm;
        break;
    default:
        break;
    }
    return EPixelDataFormat::RGBA_SF32;
}

void CubeTexture::init(CubeTexture *texture)
{
    ImageResourceCreateInfo imageCI{ .imageFormat = texture->dataFormat, .dimensions = texture->textureSize, .numOfMips = texture->mipCount };

    ENQUEUE_RENDER_COMMAND(InitCubeTexture)
    (
        [texture, imageCI](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            texture->textureResource = graphicsHelper->createCubeImage(graphicsInstance, imageCI);
            texture->textureResource->setResourceName(texture->textureName);
            texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
            texture->textureResource->setSampleCounts(texture->getSampleCount());

            texture->textureResource->init();
            cmdList->setupInitialLayout(texture->textureResource);
        }
    );
}

void CubeTexture::destroy(CubeTexture *texture)
{
    ImageResourceRef textureResource = texture->textureResource;
    texture->textureResource.reset();

    ENQUEUE_RENDER_COMMAND(DestroyCubeTexture)
    (
        [textureResource](IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            textureResource->release();
        }
    );
}

CubeTexture *CubeTexture::createTexture(const CubeTextureCreateParams &createParams)
{
    CubeTexture *texture = new CubeTexture();

    texture->mipCount = createParams.mipCount;

    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1 + Math::floor(Math::log2((float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount
        );
    }
    texture->textureSize = UInt3(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1); // MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    texture->dataFormat = determineDataFormat(createParams.dataFormat);

    CubeTexture::init(texture);

    return texture;
}

void CubeTexture::destroyTexture(CubeTexture *cubeTexture)
{
    CubeTexture::destroy(cubeTexture);
    delete cubeTexture;
}

//////////////////////////////////////////////////////////////////////////
/// Cube texture Read/Write
//////////////////////////////////////////////////////////////////////////

void CubeTextureRW::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_RENDER_COMMAND(ReinitCubeTextureRW)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                cmdList->setupInitialLayout(textureResource);
            }
            else
            {
                CubeTextureRW::init(this);
            }
        }
    );
}

void CubeTextureRW::init(CubeTextureRW *texture)
{
    ImageResourceCreateInfo imageCI{ .imageFormat = texture->dataFormat, .dimensions = texture->textureSize, .numOfMips = texture->mipCount };

    ENQUEUE_RENDER_COMMAND(InitCubeTextureRW)
    (
        [texture, imageCI](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            texture->textureResource = graphicsHelper->createCubeImage(graphicsInstance, imageCI);
            texture->textureResource->setResourceName(texture->textureName);
            texture->textureResource->setShaderUsage(EImageShaderUsage::Writing | (texture->bWriteOnly ? 0 : EImageShaderUsage::Sampling));
            texture->textureResource->setSampleCounts(texture->getSampleCount());

            texture->textureResource->init();
            cmdList->setupInitialLayout(texture->textureResource);
        }
    );
}

CubeTextureRW *CubeTextureRW::createTexture(const CubeTextureRWCreateParams &createParams)
{
    CubeTextureRW *texture = new CubeTextureRW();

    texture->mipCount = createParams.mipCount;

    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1 + Math::floor(Math::log2((float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount
        );
    }
    texture->textureSize = UInt3(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1); // MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    texture->bWriteOnly = createParams.bWriteOnly;
    texture->dataFormat = determineDataFormat(createParams.dataFormat);

    CubeTextureRW::init(texture);

    return texture;
}

void CubeTextureRW::destroyTexture(CubeTextureRW *cubeTexture) { CubeTexture::destroyTexture(cubeTexture); }