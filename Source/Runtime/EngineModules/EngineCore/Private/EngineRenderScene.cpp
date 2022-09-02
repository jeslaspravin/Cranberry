/*!
 * \file EngineRenderScene.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EngineRenderScene.h"
#include "Types/Camera/Camera.h"
#include "Classes/World.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "IRenderInterfaceModule.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Rendering/CommandBuffer.h"

namespace ERendererIntermTexture
{
EPixelDataFormat::Type getPixelFormat(Type textureType)
{
    switch (textureType)
    {
    case ERendererIntermTexture::GBufferDiffuse:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[0];
        break;
    case ERendererIntermTexture::GBufferNormal:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[1];
        break;
    case ERendererIntermTexture::GBufferARM:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[2];
        break;
    case ERendererIntermTexture::TempTest:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[0];
        break;
    default:
        break;
    }
    return EPixelDataFormat::BGRA_U8_Norm;
}

const TChar *toString(Type textureType)
{
    switch (textureType)
    {
    case ERendererIntermTexture::GBufferDiffuse:
        return TCHAR("GBuffer_Diffuse");
        break;
    case ERendererIntermTexture::GBufferNormal:
        return TCHAR("GBuffer_Normal");
        break;
    case ERendererIntermTexture::GBufferARM:
        return TCHAR("GBuffer_ARM");
        break;
    case ERendererIntermTexture::TempTest:
        return TCHAR("TempTest");
        break;
    default:
        break;
    }
    return TCHAR("InvalidIntermFormat");
}

} // namespace ERendererIntermTexture

//////////////////////////////////////////////////////////////////////////
// SceneRenderTexturePool Implementations
//////////////////////////////////////////////////////////////////////////

const RendererIntermTexture &SceneRenderTexturePool::getTexture(
    IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size3D size, PoolTextureDesc textureDesc
)
{
    ASSERT_INSIDE_RENDERTHREAD();

    if (const RendererIntermTexture *foundTexture = getTexture(cmdList, rtType, size))
    {
        return *foundTexture;
    }

    // If we are not doing MSAA then the MIP count also must be 1. As texture and RT will be same
    const bool bMSAATexture = textureDesc.sampleCount != EPixelSampleCount::SampleCount1;
    debugAssert(bMSAATexture || textureDesc.mipCount == 1);
    ImageResourceCreateInfo ci;
    ci.dimensions = size;
    ci.imageFormat = ERendererIntermTexture::getPixelFormat(rtType);
    ci.numOfMips = 1;
    ci.layerCount = textureDesc.layerCount;

    TextureList::size_type idx = textures.get();
    textures[idx].clearCounter = bufferingCount;
    RendererIntermTexture &texture = textures[idx].intermTexture;
    texture.rtTexture = texture.resolvedTexture
        = IRenderInterfaceModule::get()->currentGraphicsHelper()->createRTImage(IRenderInterfaceModule::get()->currentGraphicsInstance(), ci);
    texture.rtTexture->setResourceName(ERendererIntermTexture::toString(rtType) + String::toString(idx));
    if (bMSAATexture)
    {
        texture.rtTexture->setSampleCounts(textureDesc.sampleCount);

        ci.numOfMips = textureDesc.mipCount;
        texture.resolvedTexture
            = IRenderInterfaceModule::get()->currentGraphicsHelper()->createImage(IRenderInterfaceModule::get()->currentGraphicsInstance(), ci);
        texture.resolvedTexture->setShaderUsage(EImageShaderUsage::Sampling);
        texture.resolvedTexture->setResourceName(ERendererIntermTexture::toString(rtType) + String::toString(idx) + TCHAR("_Resolved"));
        texture.resolvedTexture->init();
    }
    else
    {
        texture.rtTexture->setShaderUsage(EImageShaderUsage::Sampling);
    }
    texture.rtTexture->init();
    LOG_VERBOSE(
        "SceneRenderTexturePool", "Allocated new RT %s(%d, %d, %d) under type %s", texture.renderTargetResource()->getResourceName(),
        texture.rtTexture->getImageSize().x, texture.rtTexture->getImageSize().y, texture.rtTexture->getImageSize().z,
        ERendererIntermTexture::toString(rtType)
    );

    // Insert into pool
    poolTextures[rtType].emplace(std::piecewise_construct, std::forward_as_tuple(size), std::forward_as_tuple(idx));
    return texture;
}

const RendererIntermTexture &SceneRenderTexturePool::getTexture(
    IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size2D size, PoolTextureDesc textureDesc
)
{
    ASSERT_INSIDE_RENDERTHREAD();
    return getTexture(cmdList, rtType, Size3D(size, 1), textureDesc);
}

const RendererIntermTexture *SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size3D size)
{
    ASSERT_INSIDE_RENDERTHREAD();

    TexturePoolListKey key{ size };

    auto texturesRange = poolTextures[rtType].equal_range(key);
    for (auto itr = texturesRange.first; itr != texturesRange.second; ++itr)
    {
        debugAssert(textures.isValid(itr->second));
        const RendererIntermTexture &intermTexture = textures[itr->second].intermTexture;
        textures[itr->second].clearCounter = bufferingCount;

        // Must be valid if present in poolTextures
        debugAssert(intermTexture.renderTargetResource().isValid());

        if (!cmdList->hasCmdsUsingResource(intermTexture.renderTargetResource())
            && (intermTexture.renderTargetResource() == intermTexture.renderResource()
                || !cmdList->hasCmdsUsingResource(intermTexture.renderResource())))
        {
            return &intermTexture;
        }
    }
    return nullptr;
}

const RendererIntermTexture *SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size2D size)
{
    ASSERT_INSIDE_RENDERTHREAD();
    return getTexture(cmdList, rtType, Size3D(size, 1));
}

void SceneRenderTexturePool::clearUnused(IRenderCommandList *cmdList)
{
    ASSERT_INSIDE_RENDERTHREAD();

    std::vector<ImageResourceRef> safeToDeleteRts;
    safeToDeleteRts.reserve(textures.size());
    for (uint32 i = 0; i != ERendererIntermTexture::MaxCount; ++i)
    {
        for (PoolTexturesMap::iterator itr = poolTextures[i].begin(); itr != poolTextures[i].end();)
        {
            debugAssert(textures.isValid(itr->second));
            TextureData &textureData = textures[itr->second];
            if (textureData.clearCounter != 0)
            {
                textureData.clearCounter--;
                ++itr;
                continue;
            }

            RendererIntermTexture &intermTexture = textureData.intermTexture;
            // Must be valid if present in poolTextures
            debugAssert(intermTexture.renderTargetResource().isValid());

            if (!cmdList->hasCmdsUsingResource(intermTexture.renderTargetResource())
                && (intermTexture.renderTargetResource() == intermTexture.renderResource()
                    || !cmdList->hasCmdsUsingResource(intermTexture.renderResource())))
            {
                safeToDeleteRts.emplace_back(intermTexture.renderTargetResource());
                if (intermTexture.renderTargetResource() != intermTexture.renderResource())
                {
                    safeToDeleteRts.emplace_back(intermTexture.renderResource());
                }

                LOG_VERBOSE(
                    "SceneRenderTexturePool", "Clearing Texture %s(%d, %d, %d) from type %s",
                    intermTexture.renderTargetResource()->getResourceName(), intermTexture.rtTexture->getImageSize().x,
                    intermTexture.rtTexture->getImageSize().y, intermTexture.rtTexture->getImageSize().z,
                    ERendererIntermTexture::toString(ERendererIntermTexture::Type(i))
                );
                textures.reset(itr->second);
                itr = poolTextures[i].erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }

    if (!safeToDeleteRts.empty())
    {
        RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
        renderMan->getGlobalRenderingContext()->clearFbsContainingRts(safeToDeleteRts);
    }
}

//////////////////////////////////////////////////////////////////////////
// EngineRenderScene Implementations
//////////////////////////////////////////////////////////////////////////

const RendererIntermTexture &EngineRenderScene::getTempTexture(IRenderCommandList *cmdList, Short2D size)
{
    debugAssert(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get() == EPixelSampleCount::SampleCount1);

    return rtPool.getTexture(cmdList, ERendererIntermTexture::TempTest, size, {});
}

String EngineRenderScene::getCmdBufferName() const { return TCHAR("EngineRenderSceneCmd_") + String::toString(frameCount % BUFFER_COUNT); }

EngineRenderScene::EngineRenderScene(cbe::World *inWorld)
    : world(inWorld)
    , rtPool(BUFFER_COUNT)
{}

copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread> EngineRenderScene::syncWorldComps(ComponentRenderSyncInfo compsUpdate)
{
    for (StringID compToRemove : compsUpdate.compsRemoved)
    {
        auto compToIdxItr = componentToRenderInfo.find(compToRemove);
        if (compToIdxItr != componentToRenderInfo.end())
        {
            SizeT renderInfoIdx = compToIdxItr->second;
            componentToRenderInfo.erase(compToIdxItr);

            destroyRenderInfo(compsRenderInfo[renderInfoIdx]);
            compsRenderInfo.reset(renderInfoIdx);
        }
    }

    // TODO(Jeslas) :
    co_return;
}

void EngineRenderScene::clearScene()
{
    // TODO(Jeslas) :
}

void EngineRenderScene::renderTheScene(Short2D viewportSize, const Camera &viewCamera)
{
    // start the rendering in Renderer
    ENQUEUE_RENDER_COMMAND(RenderScene)
    (
        [viewCamera, viewportSize,
         this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            cmdList->finishCmd(getCmdBufferName());

            lastRT = getTempTexture(cmdList, viewportSize);
            renderTheSceneRenderThread(viewportSize, viewCamera, cmdList, graphicsInstance, graphicsHelper);
            // Clear once every buffer cycle
            if ((frameCount % BUFFER_COUNT) == 0)
            {
                rtPool.clearUnused(cmdList);
            }
            frameCount++;
        }
    );
}

const IRenderTargetTexture *EngineRenderScene::getLastRTResolved() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return &lastRT;
}

void EngineRenderScene::onLastRTCopied()
{
    // TODO(Jeslas) :
}

void EngineRenderScene::createRenderInfo(cbe::RenderableComponent *comp, ComponentRenderInfo &outRenderInfo) const
{
    // TODO(Jeslas) :
}

void EngineRenderScene::destroyRenderInfo(const ComponentRenderInfo &renderInfo) const
{
    // TODO(Jeslas) :
}

void EngineRenderScene::renderTheSceneRenderThread(
    Short2D viewportSize, const Camera &viewCamera, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
    const GraphicsHelperAPI *graphicsHelper
)
{
    IRenderInterfaceModule *renderModule = IRenderInterfaceModule::get();

    LocalPipelineContext pipelineCntxt;
    pipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
    pipelineCntxt.materialName = TCHAR("ClearRT");
    renderModule->getRenderManager()->preparePipelineContext(&pipelineCntxt, { &lastRT });
    if (!clearParams.isValid())
    {
        clearParams = graphicsHelper->createShaderParameters(graphicsInstance, pipelineCntxt.getPipeline()->getParamLayoutAtSet(0));
        clearParams->setResourceName(TCHAR("ClearParams"));
        clearParams->setVector4Param(STRID("clearColor"), LinearColorConst::BLACK);
        clearParams->init();
    }

    GraphicsPipelineState pipelineState;
    pipelineState.pipelineQuery.drawMode = EPolygonDrawMode::Fill;
    pipelineState.pipelineQuery.cullingMode = ECullingMode::BackFace;

    QuantizedBox2D viewport{
        {             0,              0},
        {viewportSize.x, viewportSize.y}
    };
    QuantizedBox2D scissor{
        {                   viewportSize.x / 4,                    viewportSize.y / 4},
        {viewportSize.x - (viewportSize.x / 4), viewportSize.y - (viewportSize.y / 4)}
    };

    RenderPassAdditionalProps additionalProps;
    additionalProps.bAllowUndefinedLayout = true;
    RenderPassClearValue clearVal;
    clearVal.colors = { LinearColorConst::PALE_BLUE };

    const GraphicsResource *cmdBuffer = cmdList->startCmd(getCmdBufferName(), EQueueFunction::Graphics, true);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, EngineRenderScene);
        cmdList->cmdBeginRenderPass(cmdBuffer, pipelineCntxt, viewport, additionalProps, clearVal);

        cmdList->cmdBindGraphicsPipeline(cmdBuffer, pipelineCntxt, pipelineState);
        cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { GlobalBuffers::getQuadTriVertexBuffer() }, { 0 });
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineCntxt, clearParams);

        cmdList->cmdDrawVertices(cmdBuffer, 0, 3);

        cmdList->cmdEndRenderPass(cmdBuffer);
    }
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo2 submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, submitInfo);
}
