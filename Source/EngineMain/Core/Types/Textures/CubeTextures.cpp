#include "CubeTextures.h"
#include "../../Math/Math.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"

uint32 CubeTexture::getMipCount() const
{
    return mipCount;
}

void CubeTexture::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitCubeTexture)(
    [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
    {
        if (textureResource->isValid())
        {
            textureResource->reinitResources();
        }
        else
        {
            CubeTexture::init(this);
        }
    });
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
	default:
		break;
    }
    return EPixelDataFormat::RGBA_SF32;
}

void CubeTexture::init(CubeTexture* texture)
{
    texture->textureResource = new GraphicsCubeImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setNumOfMips(texture->mipCount);

    ENQUEUE_COMMAND(InitCubeTexture)(
    [texture](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
    {
        texture->textureResource->init();
        cmdList->setupInitialLayout(texture->textureResource);
    });
}

void CubeTexture::destroy(CubeTexture* texture)
{
    ImageResource* textureResource = texture->textureResource;
    texture->textureResource = nullptr;

    ENQUEUE_COMMAND(DestroyCubeTexture)(
    [textureResource](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
    {
        textureResource->release();
        delete textureResource;
    });
}

CubeTexture* CubeTexture::createTexture(const CubeTextureCreateParams& createParams)
{
    CubeTexture* texture = new CubeTexture();

    texture->mipCount = createParams.mipCount;

    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    texture->dataFormat = determineDataFormat(createParams.dataFormat);

    CubeTexture::init(texture);

    return texture;
}

void CubeTexture::destroyTexture(CubeTexture* cubeTexture)
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

    ENQUEUE_COMMAND(ReinitCubeTextureRW)(
    [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
    {
        if (textureResource->isValid())
        {
            textureResource->reinitResources();
        }
        else
        {
            CubeTextureRW::init(this);
        }
    });
}

void CubeTextureRW::init(CubeTextureRW* texture)
{
    texture->textureResource = new GraphicsCubeImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Writing | (texture->bWriteOnly? 0 : EImageShaderUsage::Sampling));
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setNumOfMips(texture->mipCount);
    
    ENQUEUE_COMMAND(InitCubeTextureRW)(
    [texture](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
    {
        texture->textureResource->init();
        cmdList->setupInitialLayout(texture->textureResource);
    });
}

CubeTextureRW* CubeTextureRW::createTexture(const CubeTextureRWCreateParams& createParams)
{
    CubeTextureRW* texture = new CubeTextureRW();

    texture->mipCount = createParams.mipCount;

    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    texture->bWriteOnly = createParams.bWriteOnly;
    texture->dataFormat = determineDataFormat(createParams.dataFormat);

    CubeTextureRW::init(texture);

    return texture;
}

void CubeTextureRW::destroyTexture(CubeTextureRW* cubeTexture)
{
    CubeTexture::destroyTexture(cubeTexture);
}