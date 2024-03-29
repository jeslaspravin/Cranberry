/*!
 * \file WorldViewport.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WorldViewport.h"
#include "Classes/EngineBase.h"
#include "Classes/WorldsManager.h"
#include "EngineRenderScene.h"
#include "Math/Camera.h"
#include "Widgets/WgRenderTarget.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "IRenderInterfaceModule.h"
#include "RenderApi/RenderManager.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

void WorldViewport::startSceneRender(Short2 viewportSize, const Camera &defaultCamera)
{
    SharedPtr<EngineRenderScene> renderScene = world.isValid() ? gCBEEngine->worldManager()->getWorldRenderScene(world.get()) : nullptr;
    if (renderScene)
    {
        renderScene->renderTheScene({ .view = defaultCamera, .viewportSize = viewportSize });
    }
}

void WorldViewport::drawBackBuffer(
    ShortRect viewport, WgRenderTarget *rt, const GraphicsResource *cmdBuffer, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
    const GraphicsHelperAPI *graphicsHelper
)
{
    SharedPtr<EngineRenderScene> renderScene = world.isValid() ? gCBEEngine->worldManager()->getWorldRenderScene(world.get()) : nullptr;
    if (renderScene)
    {
        const IRenderTargetTexture *resolvedRt = renderScene->getLastRTResolved();

        // Find if any existing params
        ShaderParametersRef paramRef;
        for (uint32 i = 0; i < ShaderParamsRing::BUFFER_COUNT; ++i)
        {
            std::pair<ImageResourceRef, ShaderParametersRef> params = resolveParams.peek(i);
            if (params.first.isValid() && params.second.isValid())
            {
                if (params.first == resolvedRt->renderResource())
                {
                    paramRef = params.second;
                    break;
                }
            }
            else
            {
                break;
            }
        }

        IRenderInterfaceModule *renderModule = IRenderInterfaceModule::get();
        LocalPipelineContext pipelineCntxt;
        pipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
        pipelineCntxt.materialName = TCHAR("DrawQuadFromTexture");
        const IRenderTargetTexture *rtPtr = rt;
        renderModule->getRenderManager()->preparePipelineContext(&pipelineCntxt, { &rtPtr, 1 });
        Int2 rtSize{ static_cast<ImageResourceRef>(rt->renderTargetResource())->getImageSize() };

        if (!paramRef.isValid())
        {
            if (resolveParams.size() == ShaderParamsRing::BUFFER_COUNT)
            {
                resolveParams.pop();
            }

            paramRef = graphicsHelper->createShaderParameters(graphicsInstance, pipelineCntxt.getPipeline()->getParamLayoutAtSet(0));
            paramRef->setResourceName(resolvedRt->renderResource()->getResourceName() + TCHAR("_Params"));
            paramRef->setTextureParam(STRID("quadTexture"), resolvedRt->renderResource(), GlobalBuffers::linearSampler());
            paramRef->init();

            resolveParams.push({ resolvedRt->renderResource(), paramRef });
        }
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, ToBackBuffer);

            GraphicsPipelineState pipelineState;
            pipelineState.pipelineQuery.drawMode = EPolygonDrawMode::Fill;
            pipelineState.pipelineQuery.cullingMode = ECullingMode::BackFace;

            IRect viewportArea{
                {viewport.minBound.x, viewport.minBound.y},
                {viewport.maxBound.x, viewport.maxBound.y}
            };
            IRect renderArea{
                {0, 0},
                rtSize
            };

            RenderPassAdditionalProps additionalProps;
            additionalProps.bAllowUndefinedLayout = true;
            RenderPassClearValue clearVal;
            clearVal.colors = { LinearColorConst::BLACK };

            cmdList->cmdBarrierResources(cmdBuffer, { &paramRef, 1 });
            cmdList->cmdBeginRenderPass(cmdBuffer, pipelineCntxt, renderArea, additionalProps, clearVal);

            cmdList->cmdBindGraphicsPipeline(cmdBuffer, pipelineCntxt, pipelineState);

            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);
            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewportArea, viewportArea);
            cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineCntxt, paramRef);

            cmdList->cmdDrawVertices(cmdBuffer, 0, 3);

            cmdList->cmdEndRenderPass(cmdBuffer);

            renderScene->onLastRTCopied();
        }
    }
}
