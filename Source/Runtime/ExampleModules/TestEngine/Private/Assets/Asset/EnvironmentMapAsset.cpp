/*!
 * \file EnvironmentMapAsset.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/Asset/EnvironmentMapAsset.h"
#include "Core/Types/Textures/CubeTextures.h"
#include "Core/Types/Textures/Texture2D.h"
#include "RenderInterface/GraphicsHelper.h"
#include "IRenderInterfaceModule.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/GlobalRenderVariables.h"

ICleanupAsset *EnvironmentMapAsset::cleanableAsset() { return this; }

void EnvironmentMapAsset::initAsset()
{
    ENQUEUE_RENDER_COMMAND(InitEnvironmentMap)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();

            ImageResourceCreateInfo hdrImageCreateInfo{
                .imageFormat = EPixelDataFormat::RGBA_SF32, .dimensions = UInt3(textureDimension, 1), .numOfMips = 0, .layerCount = 1
            };
            ImageResourceRef hdrImage = graphicsHelper->createImage(graphicsInstance, hdrImageCreateInfo);
            hdrImage->setResourceName(TCHAR("HDR_temp_image"));
            hdrImage->setShaderUsage(EImageShaderUsage::Sampling);
            hdrImage->setSampleCounts(EPixelSampleCount::SampleCount1);
            hdrImage->init();
            // Copying
            {
                CopyPixelsToImageInfo copyInfo;
                copyInfo.bGenerateMips = true;
                copyInfo.mipFiltering = ESamplerFiltering::Linear;
                copyInfo.dstOffset = copyInfo.srcOffset = UInt3(0);
                copyInfo.extent = hdrImage->getImageSize();
                copyInfo.subres.layersCount = 1;
                copyInfo.subres.baseMip = copyInfo.subres.baseLayer = 0;
                cmdList->copyToImage(hdrImage, tempPixelData, copyInfo);
            }

            // Create Textures
            CubeTextureCreateParams createParams;
            createParams.dataFormat = ECubeTextureFormat::CT_F16;
            createParams.mipCount = 1;
            createParams.textureSize = UInt2(GlobalRenderVariables::MAX_ENV_MAP_SIZE);
            createParams.textureName = assetName() + TCHAR("_EnvMap");
            envMap = TextureBase::createTexture<CubeTexture>(createParams);

            // Diffuse Irradiance
            createParams.dataFormat = ECubeTextureFormat::CT_F32;
            // 1024/16 = 64, Scaled in this ratio
            createParams.textureSize = UInt2(GlobalRenderVariables::MAX_ENV_MAP_SIZE / 16u);
            createParams.textureName = assetName() + TCHAR("_DifIrrad");
            diffuseIrradMap = TextureBase::createTexture<CubeTexture>(createParams);

            // Pre-filtered specular map
            createParams.dataFormat = ECubeTextureFormat::CT_F16;
            createParams.textureSize = UInt2(GlobalRenderVariables::MAX_ENV_MAP_SIZE / 2u);
            createParams.mipCount = GlobalRenderVariables::MAX_PREFILTERED_CUBE_MIPS;
            createParams.textureName = assetName() + TCHAR("_FilteredSpec");
            specularIrradMap = TextureBase::createTexture<CubeTexture>(createParams);

            {
                UInt3 subgrpSize{ 16, 16, 1 };

                // Create intermediates
                CubeTextureRWCreateParams rwCreateParams;
                rwCreateParams.bWriteOnly = true;
                rwCreateParams.dataFormat = ECubeTextureFormat::CT_F16;
                rwCreateParams.mipCount = 1;
                rwCreateParams.textureSize = envMap->getTextureSize();
                rwCreateParams.textureName = TCHAR("CubeMapIntermediate");
                CubeTextureRW *writeIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

                // Diffuse irradiance intermediate
                rwCreateParams.dataFormat = ECubeTextureFormat::CT_F32;
                rwCreateParams.textureSize = diffuseIrradMap->getTextureSize();
                rwCreateParams.textureName = TCHAR("DiffuseIrradIntermediate");
                CubeTextureRW *diffIrradIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

                rwCreateParams.dataFormat = ECubeTextureFormat::CT_F16;
                rwCreateParams.textureSize = specularIrradMap->getTextureSize();
                rwCreateParams.mipCount = GlobalRenderVariables::MAX_PREFILTERED_CUBE_MIPS;
                rwCreateParams.textureName = TCHAR("SpecularIrradIntermediate");
                CubeTextureRW *specIrradIntermediate = TextureBase::createTexture<CubeTextureRW>(rwCreateParams);

                SamplerCreateInfo samplerCI{
                    .filtering = ESamplerFiltering::Linear,
                    .mipFiltering = ESamplerFiltering::Linear,
                    .tilingMode = { ESamplerTilingMode::Repeat, ESamplerTilingMode::Repeat, ESamplerTilingMode::Repeat },
                    .mipLodRange = { 0, float(hdrImage->getNumOfMips()) }
                };
                SamplerRef sampler = graphicsHelper->createSampler(graphicsInstance, samplerCI);
                sampler->init();

                // Create Env map from HDRI
                LocalPipelineContext hdriToCubeContext;
                hdriToCubeContext.materialName = TCHAR("HDRIToCube_16x16x1");
                renderMan->preparePipelineContext(&hdriToCubeContext);
                ShaderParametersRef hdriToCubeParams
                    = graphicsHelper->createShaderParameters(graphicsInstance, hdriToCubeContext.getPipeline()->getParamLayoutAtSet(0), {});
                hdriToCubeParams->setTextureParam(TCHAR("outCubeMap"), writeIntermediate->getTextureResource());
                hdriToCubeParams->setTextureParam(TCHAR("hdri"), hdrImage, sampler);
                hdriToCubeParams->init();

                // Env to Diffuse Irradiance
                LocalPipelineContext envToDiffIrradContext;
                envToDiffIrradContext.materialName = TCHAR("EnvToDiffuseIrradiance_4x4x1");
                renderMan->preparePipelineContext(&envToDiffIrradContext);
                ShaderParametersRef envToDiffIrradParams
                    = graphicsHelper->createShaderParameters(graphicsInstance, envToDiffIrradContext.getPipeline()->getParamLayoutAtSet(0), {});
                envToDiffIrradParams->setTextureParam(TCHAR("outDiffuseIrradiance"), diffIrradIntermediate->getTextureResource());
                envToDiffIrradParams->setTextureParam(TCHAR("envMap"), envMap->getTextureResource(), sampler);
                envToDiffIrradParams->init();

                // HDRI to pre-filtered specular map
                LocalPipelineContext hdriToPrefilteredSpecContext;
                hdriToPrefilteredSpecContext.materialName = TCHAR("HDRIToPrefilteredSpecMap_16x16x1");
                renderMan->preparePipelineContext(&hdriToPrefilteredSpecContext);
                ShaderParametersRef hdriToPrefilteredSpecParams = graphicsHelper->createShaderParameters(
                    graphicsInstance, hdriToPrefilteredSpecContext.getPipeline()->getParamLayoutAtSet(0), {}
                );
                for (uint32 i = 0; i < specIrradIntermediate->getMipCount(); ++i)
                {
                    ImageViewInfo viewInfo;
                    viewInfo.viewSubresource.baseMip = i;
                    viewInfo.viewSubresource.mipCount = 1;
                    hdriToPrefilteredSpecParams->setTextureParam(TCHAR("outPrefilteredSpecMap"), specIrradIntermediate->getTextureResource(), i
                    );
                    hdriToPrefilteredSpecParams->setTextureParamViewInfo(TCHAR("outPrefilteredSpecMap"), viewInfo, i);
                }
                hdriToPrefilteredSpecParams->setTextureParam(TCHAR("hdri"), hdrImage, sampler);
                hdriToPrefilteredSpecParams->init();

                // Start creating maps
                const GraphicsResource *createEnvCmdBuffer
                    = cmdList->startCmd(TCHAR("CreateEnvMap_") + assetName(), EQueueFunction::Graphics, false);

                cmdList->cmdBarrierResources(createEnvCmdBuffer, { &hdriToCubeParams, 1 });
                cmdList->cmdBindComputePipeline(createEnvCmdBuffer, hdriToCubeContext);
                cmdList->cmdBindDescriptorsSets(createEnvCmdBuffer, hdriToCubeContext, hdriToCubeParams);
                subgrpSize = static_cast<const ComputeShaderConfig *>(hdriToCubeContext.getPipeline()->getShaderResource()->getShaderConfig())
                                 ->getSubGroupSize();
                cmdList->cmdDispatch(
                    createEnvCmdBuffer, writeIntermediate->getTextureSize().x / subgrpSize.x,
                    writeIntermediate->getTextureSize().y / subgrpSize.y
                );

                CopyImageInfo copyInfo;
                copyInfo.extent = UInt3(writeIntermediate->getTextureSize(), 1);
                cmdList->cmdCopyOrResolveImage(
                    createEnvCmdBuffer, writeIntermediate->getTextureResource(), envMap->getTextureResource(), copyInfo, copyInfo
                );

                ShaderParametersRef barrierParams[] = { envToDiffIrradParams, hdriToPrefilteredSpecParams };
                cmdList->cmdBarrierResources(createEnvCmdBuffer, barrierParams);
                cmdList->cmdBindComputePipeline(createEnvCmdBuffer, envToDiffIrradContext);
                cmdList->cmdBindDescriptorsSets(createEnvCmdBuffer, envToDiffIrradContext, envToDiffIrradParams);
                subgrpSize
                    = static_cast<const ComputeShaderConfig *>(envToDiffIrradContext.getPipeline()->getShaderResource()->getShaderConfig())
                          ->getSubGroupSize();
                cmdList->cmdDispatch(
                    createEnvCmdBuffer, diffIrradIntermediate->getTextureSize().x / subgrpSize.x,
                    diffIrradIntermediate->getTextureSize().y / subgrpSize.y
                );

                copyInfo.extent = UInt3(diffIrradIntermediate->getTextureSize(), 1);
                cmdList->cmdCopyOrResolveImage(
                    createEnvCmdBuffer, diffIrradIntermediate->getTextureResource(), diffuseIrradMap->getTextureResource(), copyInfo, copyInfo
                );

                cmdList->cmdBindComputePipeline(createEnvCmdBuffer, hdriToPrefilteredSpecContext);
                std::pair<String, std::any> pushConsts[] = {
                    {TCHAR("sourceSize"), envMap->getTextureSize().x}
                };
                cmdList->cmdPushConstants(createEnvCmdBuffer, hdriToPrefilteredSpecContext, pushConsts);
                cmdList->cmdBindDescriptorsSets(createEnvCmdBuffer, hdriToPrefilteredSpecContext, hdriToPrefilteredSpecParams);
                subgrpSize = static_cast<const ComputeShaderConfig *>(
                                 hdriToPrefilteredSpecContext.getPipeline()->getShaderResource()->getShaderConfig()
                )
                                 ->getSubGroupSize();
                cmdList->cmdDispatch(
                    createEnvCmdBuffer, specIrradIntermediate->getTextureSize().x / subgrpSize.x,
                    specIrradIntermediate->getTextureSize().y / subgrpSize.y
                );

                copyInfo.extent = UInt3(specIrradIntermediate->getTextureSize(), 1);
                cmdList->cmdCopyOrResolveImage(
                    createEnvCmdBuffer, specIrradIntermediate->getTextureResource(), specularIrradMap->getTextureResource(), copyInfo, copyInfo
                );
                ImageResourceRef imgsToTransition[] = { specularIrradMap->getTextureResource(), diffuseIrradMap->getTextureResource() };
                cmdList->cmdTransitionLayouts(createEnvCmdBuffer, imgsToTransition);

                // Record here
                cmdList->endCmd(createEnvCmdBuffer);

                CommandSubmitInfo2 submitInfo;
                submitInfo.cmdBuffers.emplace_back(createEnvCmdBuffer);
                cmdList->submitCmd(EQueuePriority::High, submitInfo);

                cmdList->finishCmd(createEnvCmdBuffer);
                cmdList->freeCmd(createEnvCmdBuffer);

                // Release intermediates
                hdrImage->release();
                hdrImage.reset();

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
        }
    );
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

void EnvironmentMapAsset::setTempPixelData(const std::vector<LinearColor> &pixelData) { tempPixelData = pixelData; }

void EnvironmentMapAsset::setTextureSize(const UInt2 &dimension) { textureDimension = dimension; }

TextureBase *EnvironmentMapAsset::getEnvironmentMap() const { return envMap; }

TextureBase *EnvironmentMapAsset::getSpecularIrradianceMap() const { return specularIrradMap; }

TextureBase *EnvironmentMapAsset::getDiffuseIrradianceMap() const { return diffuseIrradMap; }
