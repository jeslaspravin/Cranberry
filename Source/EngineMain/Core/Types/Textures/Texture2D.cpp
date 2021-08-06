#include "Texture2D.h"
#include "../../Logger/Logger.h"
#include "../../Engine/Config/EngineGlobalConfigs.h"
#include "../../../RenderInterface/Rendering/CommandBuffer.h"
#include "../../../RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "../../../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../Math/Math.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"

uint32 Texture2D::getMipCount() const
{
    return mipCount;
}

void Texture2D::setData(const std::vector<class Color>& newData, const Color& defaultColor)
{
    rawData.resize(Math::max((uint32)newData.size(), textureSize.x * textureSize.y));

    dataFormat = EPixelDataFormat::BGRA_U8_Norm;
    memcpy(rawData.data(), newData.data(), newData.size() * sizeof(Color));
    for (uint32 copyIndex = uint32(newData.size()); copyIndex < static_cast<uint32>(rawData.size()); ++copyIndex)
    {
        rawData[copyIndex] = defaultColor;
    }

    markResourceDirty();
}

bool Texture2D::isSrgb() const
{
    return dataFormat == EPixelDataFormat::BGRA_U8_SRGB
        || dataFormat == EPixelDataFormat::RG_U8_SRGB 
        || dataFormat == EPixelDataFormat::R_U8_SRGB;
}

EPixelDataFormat::Type Texture2D::determineDataFormat(bool bIsSrgb, bool bIsNormalMap, uint8 componentCount)
{
    EPixelDataFormat::Type dataFormat;
    if (bIsNormalMap)
    {
        // EPixelDataFormat::A2BGR10_U32_NormPacked is taking too long to be copied due to bit manipulations
        // Change this to EPixelDataFormat::A2BGR10_U32_NormPacked after custom serialized assets are added to engine
        dataFormat = EPixelDataFormat::BGRA_U8_Norm;
        //dataFormat = EPixelDataFormat::A2BGR10_U32_NormPacked;
    }
    else
    {
        switch (componentCount)
        {
        case 1:
            dataFormat = bIsSrgb ? EPixelDataFormat::R_U8_SRGB : EPixelDataFormat::R_U8_Norm;
            break;
        case 2:
            dataFormat = bIsSrgb ? EPixelDataFormat::RG_U8_SRGB : EPixelDataFormat::RG_U8_Norm;
            break;
        case 3:
        case 4:
        default:
            dataFormat = bIsSrgb ? EPixelDataFormat::BGRA_U8_SRGB : EPixelDataFormat::BGRA_U8_Norm;
            break;
        }
    }
    return dataFormat;
}

Texture2D* Texture2D::createTexture(const Texture2DCreateParams& createParams)
{
    Texture2D* texture = new Texture2D();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->setData(createParams.colorData, createParams.defaultColor);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    Texture2D::init(texture, createParams.bIsNormalMap, createParams.bIsSrgb, createParams.componentsCount);
    return texture;
}

void Texture2D::destroyTexture(Texture2D* texture2D)
{
    Texture2D::destroy(texture2D);
    delete texture2D;
    texture2D = nullptr;
}

void Texture2D::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitTexture2D)(
        [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            const EPixelDataFormat::PixelFormatInfo * formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
            bool bIsNormalMap = (formatInfo->componentSize[uint8(EPixelComponent::R)] > 8);
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                if (bIsNormalMap)
                {
                    cmdList->copyToImageLinearMapped(textureResource, rawData);
                }
                else
                {
                    cmdList->copyToImage(textureResource, rawData);
                }
            }
            else
            {
                Texture2D::init(this, bIsNormalMap, isSrgb(), formatInfo->componentCount);
            }
        }
    );
}

void Texture2D::init(Texture2D* texture, bool bIsNormalMap, bool bIsSrgb, uint8 componentCount)
{
    EPixelDataFormat::Type dataFormat = Texture2D::determineDataFormat(bIsSrgb, bIsNormalMap, componentCount);    

    texture->textureResource = new GraphicsImageResource(dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setLayerCount(1);
    texture->textureResource->setNumOfMips(texture->mipCount);

    ENQUEUE_COMMAND(InitTexture2D)(
        [texture, bIsNormalMap](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            texture->textureResource->init();
            if (bIsNormalMap)
            {
                cmdList->copyToImageLinearMapped(texture->textureResource, texture->rawData);
            }
            else
            {
                cmdList->copyToImage(texture->textureResource, texture->rawData);
            }
        }
    );
}

void Texture2D::destroy(Texture2D* texture)
{
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyTexture2D)(
        [textureResource](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            textureResource->release();
            delete textureResource;
        }
    );

    texture->textureResource = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// RW texture
//////////////////////////////////////////////////////////////////////////

void Texture2DRW::destroy(Texture2DRW* texture)
{
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyTexture2D)(
        [textureResource](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            textureResource->release();
            delete textureResource;
        });

    texture->textureResource = nullptr;
}

void Texture2DRW::init(Texture2DRW* texture)
{
    texture->textureResource = new GraphicsImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(texture->bIsWriteOnly? EImageShaderUsage::Writing : EImageShaderUsage::Sampling | EImageShaderUsage::Writing);
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setLayerCount(1);
    texture->textureResource->setNumOfMips(texture->mipCount);

    ENQUEUE_COMMAND(InitTexture2D)(
        [texture](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            texture->textureResource->init();
            cmdList->setupInitialLayout(texture->textureResource);
            if (!texture->bIsWriteOnly)
            {
                cmdList->copyToImage(texture->textureResource, texture->rawData);
            }
        }
    );
}

void Texture2DRW::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitTexture2D)(
        [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                cmdList->setupInitialLayout(textureResource);
                if (!bIsWriteOnly)
                {
                    cmdList->copyToImage(textureResource, rawData);
                }
            }
            else
            {
                Texture2DRW::init(this);
            }
        }
    );
}

void Texture2DRW::destroyTexture(Texture2DRW* texture2D)
{
    if(texture2D)
    {
        Texture2DRW::destroy(texture2D);
        delete texture2D;
        texture2D = nullptr;
    }
}

Texture2DRW* Texture2DRW::createTexture(const Texture2DRWCreateParams& createParams)
{
    Texture2DRW* texture = new Texture2DRW();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsWriteOnly = createParams.bIsWriteOnly;
    texture->setData(createParams.colorData, createParams.defaultColor, false);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);
    texture->dataFormat = createParams.format;

    Texture2DRW::init(texture);
    return texture;
}

void Texture2DRW::setData(const std::vector<class Color>& newData, const Color& defaultColor, bool bIsSrgb)
{
    rawData.resize(Math::max((uint32)newData.size(), textureSize.x * textureSize.y));

    int32 copyIndex = 0;
    if (bIsSrgb)
    {
        dataFormat = EPixelDataFormat::RGBA_U8_SRGB;
        for (; copyIndex < static_cast<int32>(newData.size()); ++copyIndex)
        {
            rawData[copyIndex] = newData[copyIndex].toSrgb();
        }
    }
    else
    {
        dataFormat = EPixelDataFormat::RGBA_U8_Norm;
        memcpy(rawData.data(), newData.data(), newData.size() * sizeof(Color));
    }
    for (; copyIndex < static_cast<int32>(rawData.size()); ++copyIndex)
    {
        rawData[copyIndex] = defaultColor;
    }

    markResourceDirty();
}

uint32 Texture2DRW::getMipCount() const
{
    return mipCount;
}


void GlobalBuffers::createTexture2Ds()
{
    Texture2DCreateParams createParams;
    createParams.bIsSrgb = false;
    createParams.defaultColor = ColorConst::BLACK;
    createParams.mipCount = 1;
    createParams.textureName = "Dummy_Black";
    createParams.textureSize = Size2D(1, 1);
    dummyBlackTexture = TextureBase::createTexture<Texture2D>(createParams);

    createParams.defaultColor = ColorConst::WHITE;
    createParams.textureName = "Dummy_White";
    dummyWhiteTexture = TextureBase::createTexture<Texture2D>(createParams);

    createParams.defaultColor = ColorConst::BLUE;
    createParams.textureName = "Dummy_Normal";
    dummyNormalTexture = TextureBase::createTexture<Texture2D>(createParams);

    if(GlobalRenderVariables::ENABLE_EXTENDED_STORAGES)
    {
        // #TODO(Jeslas) : Create better read only LUT
        Texture2DRWCreateParams rwCreateParams;
        rwCreateParams.defaultColor = ColorConst::BLACK;
        rwCreateParams.mipCount = 1;
        rwCreateParams.textureName = "LUT_IntegratedBRDF";
        rwCreateParams.textureSize = Size2D(EngineSettings::maxEnvMapSize / 2u);
        rwCreateParams.format = EPixelDataFormat::RG_SF16;
        integratedBRDF = TextureBase::createTexture<Texture2DRW>(rwCreateParams);
    }
    else
    {
        Logger::error("GlobalBuffers", "%s(): Cannot create integrated BRDF LUT, RG_SF16 is not supported format", __func__);
        integratedBRDF = nullptr;
    }
}

void GlobalBuffers::generateTexture2Ds()
{
    ENQUEUE_COMMAND(IntegrateBRDF)(
        [](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            LocalPipelineContext integrateBrdfContext;
            integrateBrdfContext.materialName = "IntegrateBRDF_16x16x1";
            gEngine->getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&integrateBrdfContext);
            SharedPtr<ShaderParameters> integrateBrdfParams = GraphicsHelper::createShaderParameters(graphicsInstance, integrateBrdfContext.getPipeline()->getParamLayoutAtSet(0), {});
            integrateBrdfParams->setTextureParam("outIntegratedBrdf", integratedBRDF->getTextureResource());
            integrateBrdfParams->init();

            const GraphicsResource* cmdBuffer = cmdList->startCmd("IntegrateBRDF", EQueueFunction::Graphics, false);
            cmdList->cmdBindComputePipeline(cmdBuffer, integrateBrdfContext);
            cmdList->cmdBindDescriptorsSets(cmdBuffer, integrateBrdfContext, { integrateBrdfParams.get() });
            Size3D subgrpSize = static_cast<const ComputeShader*>(integrateBrdfContext.getPipeline()->getShaderResource())->getSubGroupSize();
            cmdList->cmdDispatch(cmdBuffer, integratedBRDF->getTextureSize().x / subgrpSize.x, integratedBRDF->getTextureSize().y / subgrpSize.y);
            cmdList->cmdTransitionLayouts(cmdBuffer, { integratedBRDF->getTextureResource() });

            cmdList->endCmd(cmdBuffer);


            CommandSubmitInfo2 submitInfo;
            submitInfo.cmdBuffers.emplace_back(cmdBuffer);
            cmdList->submitWaitCmd(EQueuePriority::High, submitInfo);

            cmdList->freeCmd(cmdBuffer);

            integrateBrdfParams->release();
            integrateBrdfParams.reset();
        }
    );
}

void GlobalBuffers::destroyTexture2Ds()
{
    TextureBase::destroyTexture<Texture2D>(dummyBlackTexture);
    dummyBlackTexture = nullptr;
    TextureBase::destroyTexture<Texture2D>(dummyWhiteTexture);
    dummyWhiteTexture = nullptr;
    TextureBase::destroyTexture<Texture2D>(dummyNormalTexture);
    dummyNormalTexture = nullptr;

    TextureBase::destroyTexture<Texture2DRW>(integratedBRDF);
}