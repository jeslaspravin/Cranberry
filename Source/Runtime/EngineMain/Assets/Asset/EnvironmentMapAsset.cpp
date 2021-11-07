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
#include "../../Core/Engine/Config/EngineGlobalConfigs.h"

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
        hdrImage.setNumOfMips(0);
        hdrImage.init();
        // Copying
        {
            CopyPixelsToImageInfo copyInfo;
            copyInfo.bGenerateMips = true;
            copyInfo.mipFiltering = ESamplerFiltering::Linear;
            copyInfo.dstOffset = copyInfo.srcOffset = Size3D(0);
            copyInfo.extent = hdrImage.getImageSize();
            copyInfo.subres.layersCount = 1;
            copyInfo.subres.baseMip = copyInfo.subres.baseLayer = 0;
            cmdList->copyToImage(&hdrImage, tempPixelData, copyInfo);
        }

        // Create Textures
        CubeTextureCreateParams createParams;
        createParams.dataFormat = ECubeTextureFormat::CT_F16;
        createParams.mipCount = 1;
        createParams.textureSize = Size2D(EngineSettings::maxEnvMapSize);
        createParams.textureName = assetName() + "_EnvMap";
        envMap = TextureBase::createTexture<CubeTexture>(createParams);

        // Diffuse Irradiance
        createParams.dataFormat = ECubeTextureFormat::CT_F32;
        // 1024/16 = 64, Scaled in this ratio
        createParams.textureSize = Size2D(EngineSettings::maxEnvMapSize / 16u);
        createParams.textureName = assetName() + "_DifIrrad";
        diffuseIrradMap = TextureBase::createTexture<CubeTexture>(createParams);

        // Pre-filtered specular map
        createParams.dataFormat = ECubeTextureFormat::CT_F16;
        createParams.textureSize = Size2D(EngineSettings::maxEnvMapSize / 2u);
        createParams.mipCount = EngineSettings::maxPrefilteredCubeMiplevels;
        createParams.textureName = assetName() + "_FilteredSpec";
        specularIrradMap = TextureBase::createTexture<CubeTexture>(createParams);

        {
            Size3D subgrpSize{ 16, 16, 1 };

            // Create intermediates
            CubeTextureRWCreateParams rwCreateParams;
            rwCreateParams.bWriteOnly = true;
            rwCreateParams.dataFormat = ECubeTextureFormat::CT_F16;
            rwCreateParams.mipCount = 1;
            rwCreateParams.textureSize = envMap->getTextureSize();
            rwCreateParams.textureName = "CubeMapIntermediate";
            CubeTextureRW* writeIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

            // Diffuse irradiance intermediate
            rwCreateParams.dataFormat = ECubeTextureFormat::CT_F32;
            rwCreateParams.textureSize = diffuseIrradMap->getTextureSize();
            rwCreateParams.textureName = "DiffuseIrradIntermediate";
            CubeTextureRW* diffIrradIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

            rwCreateParams.dataFormat = ECubeTextureFormat::CT_F16;
            rwCreateParams.textureSize = specularIrradMap->getTextureSize();
            rwCreateParams.mipCount = EngineSettings::maxPrefilteredCubeMiplevels;
            rwCreateParams.textureName = "SpecularIrradIntermediate";
            CubeTextureRW* specIrradIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

            SharedPtr<SamplerInterface> sampler = GraphicsHelper::createSampler(graphicsInstance, "EnvMapSampler", ESamplerTilingMode::Repeat, ESamplerFiltering::Linear
                , float(hdrImage.getNumOfMips()));

            // Create Env map from HDRI
            LocalPipelineContext hdriToCubeContext;
            hdriToCubeContext.materialName = "HDRIToCube_16x16x1";
            gEngine->getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&hdriToCubeContext);
            SharedPtr<ShaderParameters> hdriToCubeParams = GraphicsHelper::createShaderParameters(graphicsInstance, hdriToCubeContext.getPipeline()->getParamLayoutAtSet(0), {});
            hdriToCubeParams->setTextureParam("outCubeMap", writeIntermediate->getTextureResource());
            hdriToCubeParams->setTextureParam("hdri", &hdrImage, sampler);
            hdriToCubeParams->init();

            // Env to Diffuse Irradiance
            LocalPipelineContext envToDiffIrradContext;
            envToDiffIrradContext.materialName = "EnvToDiffuseIrradiance_4x4x1";
            gEngine->getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&envToDiffIrradContext);
            SharedPtr<ShaderParameters> envToDiffIrradParams = GraphicsHelper::createShaderParameters(graphicsInstance, envToDiffIrradContext.getPipeline()->getParamLayoutAtSet(0), {});
            envToDiffIrradParams->setTextureParam("outDiffuseIrradiance", diffIrradIntermediate->getTextureResource());
            envToDiffIrradParams->setTextureParam("envMap", envMap->getTextureResource(), sampler);
            envToDiffIrradParams->init();

            // HDRI to pre-filtered specular map
            LocalPipelineContext hdriToPrefilteredSpecContext;
            hdriToPrefilteredSpecContext.materialName = "HDRIToPrefilteredSpecMap_16x16x1";
            gEngine->getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&hdriToPrefilteredSpecContext);
            SharedPtr<ShaderParameters> hdriToPrefilteredSpecParams = GraphicsHelper::createShaderParameters(graphicsInstance, hdriToPrefilteredSpecContext.getPipeline()->getParamLayoutAtSet(0), {});
            for (uint32 i = 0; i < specIrradIntermediate->getMipCount(); ++i)
            {
                ImageViewInfo viewInfo;
                viewInfo.viewSubresource.baseMip = i;
                viewInfo.viewSubresource.mipCount = 1;
                hdriToPrefilteredSpecParams->setTextureParam("outPrefilteredSpecMap", specIrradIntermediate->getTextureResource(), i);
                hdriToPrefilteredSpecParams->setTextureParamViewInfo("outPrefilteredSpecMap", viewInfo, i);
            }
            hdriToPrefilteredSpecParams->setTextureParam("hdri", &hdrImage, sampler);
            hdriToPrefilteredSpecParams->init();

            // Start creating maps
            const GraphicsResource* createEnvCmdBuffer = cmdList->startCmd("CreateEnvMap_" + assetName(), EQueueFunction::Graphics, false);

            cmdList->cmdBarrierResources(createEnvCmdBuffer, { hdriToCubeParams.get() });
            cmdList->cmdBindComputePipeline(createEnvCmdBuffer, hdriToCubeContext);
            cmdList->cmdBindDescriptorsSets(createEnvCmdBuffer, hdriToCubeContext, hdriToCubeParams.get());
            subgrpSize = static_cast<const ComputeShader*>(hdriToCubeContext.getPipeline()->getShaderResource())->getSubGroupSize();
            cmdList->cmdDispatch(createEnvCmdBuffer, writeIntermediate->getTextureSize().x / subgrpSize.x, writeIntermediate->getTextureSize().y / subgrpSize.y);

            CopyImageInfo copyInfo;
            copyInfo.extent = Size3D(writeIntermediate->getTextureSize(), 1);
            cmdList->cmdCopyOrResolveImage(createEnvCmdBuffer, writeIntermediate->getTextureResource(), envMap->getTextureResource(), copyInfo, copyInfo);

            cmdList->cmdBarrierResources(createEnvCmdBuffer, { envToDiffIrradParams.get(), hdriToPrefilteredSpecParams.get() });
            cmdList->cmdBindComputePipeline(createEnvCmdBuffer, envToDiffIrradContext);
            cmdList->cmdBindDescriptorsSets(createEnvCmdBuffer, envToDiffIrradContext, envToDiffIrradParams.get());
            subgrpSize = static_cast<const ComputeShader*>(envToDiffIrradContext.getPipeline()->getShaderResource())->getSubGroupSize();
            cmdList->cmdDispatch(createEnvCmdBuffer, diffIrradIntermediate->getTextureSize().x / subgrpSize.x, diffIrradIntermediate->getTextureSize().y / subgrpSize.y);

            copyInfo.extent = Size3D(diffIrradIntermediate->getTextureSize(), 1);
            cmdList->cmdCopyOrResolveImage(createEnvCmdBuffer, diffIrradIntermediate->getTextureResource(), diffuseIrradMap->getTextureResource(), copyInfo, copyInfo);

            cmdList->cmdBindComputePipeline(createEnvCmdBuffer, hdriToPrefilteredSpecContext);
            cmdList->cmdPushConstants(createEnvCmdBuffer, hdriToPrefilteredSpecContext, { {"sourceSize", envMap->getTextureSize().x } });
            cmdList->cmdBindDescriptorsSets(createEnvCmdBuffer, hdriToPrefilteredSpecContext, hdriToPrefilteredSpecParams.get());
            subgrpSize = static_cast<const ComputeShader*>(hdriToPrefilteredSpecContext.getPipeline()->getShaderResource())->getSubGroupSize();
            cmdList->cmdDispatch(createEnvCmdBuffer, specIrradIntermediate->getTextureSize().x / subgrpSize.x, specIrradIntermediate->getTextureSize().y / subgrpSize.y);

            copyInfo.extent = Size3D(specIrradIntermediate->getTextureSize(), 1);
            cmdList->cmdCopyOrResolveImage(createEnvCmdBuffer, specIrradIntermediate->getTextureResource(), specularIrradMap->getTextureResource(), copyInfo, copyInfo);
            cmdList->cmdTransitionLayouts(createEnvCmdBuffer, { specularIrradMap->getTextureResource(), diffuseIrradMap->getTextureResource() });

            // Record here
            cmdList->endCmd(createEnvCmdBuffer);;

            CommandSubmitInfo2 submitInfo;
            submitInfo.cmdBuffers.emplace_back(createEnvCmdBuffer);
            cmdList->submitCmd(EQueuePriority::High, submitInfo);

            cmdList->finishCmd(createEnvCmdBuffer);
            cmdList->freeCmd(createEnvCmdBuffer);

            // Release intermediates
            hdrImage.release();

            TextureBase::destroyTexture<CubeTextureRW>(writeIntermediate);
            TextureBase::destroyTexture<CubeTextureRW>(diffIrradIntermediate);
            TextureBase::destroyTexture<CubeTextureRW>(specIrradIntermediate);
            sampler->release();
            sampler.reset();
            hdriToCubeParams->release();
            hdriToCubeParams.reset();
            envToDiffIrradParams->release();
            envToDiffIrradParams.reset();
            hdriToPrefilteredSpecParams->release();
            hdriToPrefilteredSpecParams.reset();
        }

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