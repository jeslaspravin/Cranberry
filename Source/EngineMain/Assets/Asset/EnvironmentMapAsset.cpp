#include "EnvironmentMapAsset.h"
#include "../../Core/Types/Textures/CubeTextures.h"
#include "../../Core/Types/Textures/Texture2D.h"
#include "../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../RenderInterface/PlatformIndependentHelper.h"
#include "../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../RenderInterface/Rendering/CommandBuffer.h"
#include "../../RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "../../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../../RenderInterface/Resources/Samplers/SamplerInterface.h"

ICleanupAsset* EnvironmentMapAsset::cleanableAsset()
{
    return this;
}

void EnvironmentMapAsset::initAsset()
{
    ENQUEUE_COMMAND(InitEnvironmentMap)(
    [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
    {    
        GraphicsImageResource hdrImage(EPixelDataFormat::RGBA_SF32);
        hdrImage.setResourceName("HDR_temp_image");
        hdrImage.setShaderUsage(EImageShaderUsage::Sampling);
        hdrImage.setSampleCounts(EPixelSampleCount::SampleCount1);
        hdrImage.setImageSize(Size3D(textureDimension, 1));
        hdrImage.setLayerCount(1);
        hdrImage.setNumOfMips(1);
        hdrImage.init();
        // Copying
        {
            CopyPixelsToImageInfo copyInfo;
            copyInfo.bGenerateMips = false;
            copyInfo.mipFiltering = ESamplerFiltering::Linear;
            copyInfo.dstOffset = copyInfo.srcOffset = Size3D(0);
            copyInfo.extent = hdrImage.getImageSize();
            copyInfo.subres.mipCount = copyInfo.subres.layersCount = 1;
            copyInfo.subres.baseMip = copyInfo.subres.baseLayer = 0;
            cmdList->copyToImage(&hdrImage, tempPixelData, copyInfo);
        }

        // Create Textures
        CubeTextureCreateParams createParams;
        createParams.dataFormat = ECubeTextureFormat::CT_F16;
        createParams.mipCount = 1;
        createParams.textureSize = Size2D(1024, 1024);
        createParams.textureName = assetName() + "_EnvMap";
        envMap = TextureBase::createTexture<CubeTexture>(createParams);

        // Diffuse Irradiance
        createParams.dataFormat = ECubeTextureFormat::CT_F32;
        createParams.textureSize = Size2D(64, 64);
        createParams.textureName = assetName() + "_DifIrrad";
        diffuseIrradMap = TextureBase::createTexture<CubeTexture>(createParams);

        // Create intermediates
        CubeTextureRWCreateParams rwCreateParams;
        rwCreateParams.bWriteOnly = true;
        rwCreateParams.dataFormat = ECubeTextureFormat::CT_F16;
        rwCreateParams.mipCount = 1;
        rwCreateParams.textureSize = Size2D(1024, 1024);
        rwCreateParams.textureName = "CubeMapIntermediate";
        CubeTextureRW* writeIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

        SharedPtr<SamplerInterface> sampler = GraphicsHelper::createSampler(graphicsInstance, "EnvMapSampler", ESamplerTilingMode::Repeat, ESamplerFiltering::Linear);

        LocalPipelineContext hdriToCubeContext;
        hdriToCubeContext.materialName = "HDRIToCube_16x16x1";
        gEngine->getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&hdriToCubeContext);
        Size3D hdriToCubeSubgrpSize = static_cast<const ComputeShader*>(hdriToCubeContext.getPipeline()->getShaderResource())->getSubGroupSize();
        SharedPtr<ShaderParameters> hdriToCubeParams = GraphicsHelper::createShaderParameters(graphicsInstance, hdriToCubeContext.getPipeline()->getParamLayoutAtSet(0), {});
        hdriToCubeParams->setTextureParam("outCubeMap", writeIntermediate->getTextureResource());
        hdriToCubeParams->setTextureParam("hdri", &hdrImage, sampler);
        hdriToCubeParams->init();

        // Start creating maps
        const GraphicsResource* cmdBuffer = cmdList->startCmd("CreateEnvMap_" + assetName(), EQueueFunction::Graphics, false);

        cmdList->cmdBarrierResources(cmdBuffer, { hdriToCubeParams.get() });
        cmdList->cmdBindComputePipeline(cmdBuffer, hdriToCubeContext);
        cmdList->cmdBindDescriptorsSets(cmdBuffer, hdriToCubeContext, hdriToCubeParams.get());
        cmdList->cmdDispatch(cmdBuffer, writeIntermediate->getTextureSize().x / hdriToCubeSubgrpSize.x, writeIntermediate->getTextureSize().y / hdriToCubeSubgrpSize.y);

        CopyImageInfo copyInfo;
        copyInfo.extent = Size3D(writeIntermediate->getTextureSize(), 1);
        cmdList->cmdCopyOrResolveImage(cmdBuffer, writeIntermediate->getTextureResource(), envMap->getTextureResource(), copyInfo, copyInfo);
        // Record here

        cmdList->cmdTransitionLayouts(cmdBuffer, { envMap->getTextureResource() });

        cmdList->endCmd(cmdBuffer);

        CommandSubmitInfo2 submitInfo;
        submitInfo.cmdBuffers.emplace_back(cmdBuffer);
        cmdList->submitWaitCmd(EQueuePriority::High, submitInfo);
        cmdList->freeCmd(cmdBuffer);

        // Release intermediates
        hdrImage.release();

        TextureBase::destroyTexture<CubeTextureRW>(writeIntermediate);
        sampler->release();
        sampler.reset();
        hdriToCubeParams->release();
        hdriToCubeParams.reset();

        tempPixelData.clear();
    });
}

void EnvironmentMapAsset::clearAsset()
{
    if (envMap)
    {
        TextureBase::destroyTexture<CubeTexture>(envMap);
        envMap = nullptr;
    }

    if (specularIrradMap)
    {
        TextureBase::destroyTexture<CubeTexture>(specularIrradMap);
        specularIrradMap = nullptr;
    }

    if (diffuseIrradMap)
    {
        TextureBase::destroyTexture<CubeTexture>(diffuseIrradMap);
        diffuseIrradMap = nullptr;
    }
}

void EnvironmentMapAsset::setTempPixelData(const std::vector<LinearColor>& pixelData)
{
    tempPixelData = pixelData;
}

void EnvironmentMapAsset::setTextureSize(const Size2D& dimension)
{
    textureDimension = dimension;
}

TextureBase* EnvironmentMapAsset::getEnvironmentMap() const
{
    return envMap;
}

TextureBase* EnvironmentMapAsset::getSpecularIrradianceMap() const
{
    return specularIrradMap;
}

TextureBase* EnvironmentMapAsset::getDiffuseIrradianceMap() const
{
    return diffuseIrradMap;
}
