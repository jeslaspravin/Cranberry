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
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "IRenderInterfaceModule.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderApi/RenderManager.h"
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
} // namespace ERendererIntermTexture

const RendererIntermTexture &EngineRenderScene::getTempTexture(IRenderCommandList *cmdList, Short2D size)
{
    debugAssert(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get() == EPixelSampleCount::SampleCount1);

    for (uint32 i = 0; i != tempTextures.size(); ++i)
    {
        if (tempTextures[i].rtTexture == nullptr || tempTextures[i].rtTexture->getImageSize().x != size.x
            || tempTextures[i].rtTexture->getImageSize().y != size.y)
        {
            ImageResourceCreateInfo ci;
            ci.dimensions = { size.x, size.y, 1 };
            ci.imageFormat = ERendererIntermTexture::getPixelFormat(ERendererIntermTexture::TempTest);
            ci.numOfMips = 1;
            ci.layerCount = 1;
            tempTextures[i].rtTexture = tempTextures[i].resolvedTexture = IRenderInterfaceModule::get()->currentGraphicsHelper()->createRTImage(
                IRenderInterfaceModule::get()->currentGraphicsInstance(), ci
            );
            tempTextures[i].rtTexture->setResourceName(TCHAR("TestEngineRenderSceneRT_") + String::toString(i));
            tempTextures[i].rtTexture->setShaderUsage(EImageShaderUsage::Sampling);
            tempTextures[i].rtTexture->init();
            return tempTextures[i];
        }

        if (!cmdList->hasCmdsUsingResource(tempTextures[i].rtTexture)
            && (tempTextures[i].resolvedTexture == tempTextures[i].rtTexture || !cmdList->hasCmdsUsingResource(tempTextures[i].resolvedTexture)
            ))
        {
            return tempTextures[i];
        }
    }
    tempTextures.resize(tempTextures.size() + 1);
    return getTempTexture(cmdList, size);
}

String EngineRenderScene::getCmdBufferName() const { return TCHAR("EngineRenderSceneCmd_") + String::toString(frameCount % BUFFER_COUNT); }

EngineRenderScene::EngineRenderScene(cbe::World *inWorld)
    : world(inWorld)
{}

copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread> EngineRenderScene::syncWorldComps(ComponentSyncInfo compsUpdate)
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
