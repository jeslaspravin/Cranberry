#include "RenderingContexts.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../PlatformIndependentHeaders.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../ShaderCore/ShaderParameters.h"
#include "../ShaderCore/ShaderParameterUtility.h"
#include "../../Core/Types/Textures/RenderTargetTextures.h"
#include "../ShaderCore/ShaderObject.h"
#include "../Resources/Pipelines.h"
#include "../../RenderApi/GBuffersAndTextures.h"
#include "../../Core/Engine/GameEngine.h"

void GlobalRenderingContextBase::initContext(IGraphicsInstance* graphicsInstance)
{
    initApiInstances();

    initShaderResources();

    initializeApiContext();
}

void GlobalRenderingContextBase::clearContext()
{
    delete pipelineFactory;
    delete shaderObjectFactory;
    delete shaderParamLayoutsFactory;

    writeAndDestroyPipelineCache();
    destroyShaderResources();

    for (const std::pair<GenericRenderPassProperties, std::vector<const Framebuffer*>>& framebuffers : rtFramebuffers)
    {
        for (Framebuffer const* const& fb : framebuffers.second)
        {
            delete fb;
        }
    }
    rtFramebuffers.clear();

    clearApiContext();
}

void GlobalRenderingContextBase::initShaderResources()
{
    if (pipelinesCache)
    {
        pipelinesCache->setResourceName("shaders");
        pipelinesCache->init();
    }

    std::set<EVertexType::Type> filledVertexInfo;
    auto vertexAttribFillLambda = [&filledVertexInfo](EVertexType::Type vertexUsed, const std::vector<ReflectInputOutput>& vertexShaderInputs)
    {
        // If not filled yet
        if (filledVertexInfo.find(vertexUsed) == filledVertexInfo.end())
        {
            filledVertexInfo.insert(vertexUsed);
            const std::vector<ShaderVertexParamInfo*>& vertexBindingsInfo = EVertexType::vertexParamInfo(vertexUsed);
            for (ShaderVertexParamInfo* vertexBindingAttributes : vertexBindingsInfo)
            {
                ShaderParameterUtility::fillRefToVertexParamInfo(*vertexBindingAttributes, vertexShaderInputs);
            }
        }
    };

    std::vector<GraphicsResource*> allShaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(allShaderResources, true, true);
    for (GraphicsResource* shader : allShaderResources)
    {
        shader->init();

        if (shader->getType()->isChildOf(DrawMeshShader::staticType()))
        {
            DrawMeshShader* drawMeshShader = static_cast<DrawMeshShader*>(shader);
            vertexAttribFillLambda(drawMeshShader->vertexUsage(), drawMeshShader->getReflection()->inputs);

            // View unique param layout
            if (sceneViewParamLayout == nullptr)
            {
                sceneViewParamLayout = shaderParamLayoutsFactory->create(drawMeshShader, 0);
                sceneViewParamLayout->init();
            }

            // Vertex unique param layout
            auto perVertParamLayoutItr = perVertexTypeLayouts.find(drawMeshShader->vertexUsage());
            if (perVertParamLayoutItr == perVertexTypeLayouts.end())
            {
                GraphicsResource* paramLayout = shaderParamLayoutsFactory->create(drawMeshShader, 1);
                paramLayout->init();
                perVertexTypeLayouts[drawMeshShader->vertexUsage()] = paramLayout;
            }

            ShaderDataCollection& shaderCollection = rawShaderObjects[shader->getResourceName()];
            if (shaderCollection.shadersParamLayout == nullptr)
            {
                shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(drawMeshShader, 2);
                shaderCollection.shadersParamLayout->init();
            }
            if (shaderCollection.shaderObject == nullptr)
            {
                shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), drawMeshShader);
            }
            GraphicsPipelineBase* graphicsPipeline = static_cast<GraphicsPipelineBase*>(pipelineFactory->create({ drawMeshShader }));
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout, 2);
            graphicsPipeline->setParamLayoutAtSet(perVertexTypeLayouts[drawMeshShader->vertexUsage()], 1);
            graphicsPipeline->setParamLayoutAtSet(sceneViewParamLayout, 0);
            graphicsPipeline->setPipelineShader(drawMeshShader);
            graphicsPipeline->setPipelineCache(pipelinesCache);
            GenericRenderPassProperties renderpassProp;
            renderpassProp.renderpassAttachmentFormat.rpFormat = drawMeshShader->renderpassUsage();
            graphicsPipeline->setRenderpassProperties(renderpassProp);

            DrawMeshShaderObject* drawMeshShaderObj = static_cast<DrawMeshShaderObject*>(shaderCollection.shaderObject);
            drawMeshShaderObj->addShader(drawMeshShader);
            drawMeshShaderObj->setPipeline(drawMeshShader, graphicsPipeline);
        }
        else if (shader->getType()->isChildOf(UniqueUtilityShader::staticType()))
        {
            UniqueUtilityShader* utilityShader = static_cast<UniqueUtilityShader*>(shader);
            vertexAttribFillLambda(utilityShader->vertexUsage(), utilityShader->getReflection()->inputs);

            ShaderDataCollection& shaderCollection = rawShaderObjects[shader->getResourceName()];
            debugAssert(shaderCollection.shaderObject == nullptr);
            debugAssert(shaderCollection.shadersParamLayout == nullptr);

            shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), utilityShader);
            shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(utilityShader, 0/*doesn't matter*/);
            shaderCollection.shadersParamLayout->init();

            GraphicsPipelineBase* graphicsPipeline = static_cast<GraphicsPipelineBase*>(pipelineFactory->create({ utilityShader }));
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout);
            // Can be parents since other child pipelines will be derived from this initial defaults
            graphicsPipeline->setPipelineShader(utilityShader);
            graphicsPipeline->setPipelineCache(pipelinesCache);
            graphicsPipeline->setCanBeParent(true);

            static_cast<UniqueUtilityShaderObject*>(shaderCollection.shaderObject)->setPipeline(graphicsPipeline->getRenderpassProperties()
                , graphicsPipeline);
        }
    }
}

void GlobalRenderingContextBase::destroyShaderResources()
{
    std::vector<GraphicsResource*> shaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(shaderResources, true, true);
    for (GraphicsResource* shader : shaderResources)
    {
        shader->release();
    }

    sceneViewParamLayout->release();
    delete sceneViewParamLayout;
    sceneViewParamLayout = nullptr;

    for (std::pair<const EVertexType::Type, GraphicsResource*>& shaderVertexParamLayout : perVertexTypeLayouts)
    {
        shaderVertexParamLayout.second->release();
        delete shaderVertexParamLayout.second;
    }
    perVertexTypeLayouts.clear();

    for (std::pair<const String, ShaderDataCollection>& shaderDataCollection : rawShaderObjects)
    {
        shaderDataCollection.second.shadersParamLayout->release();
        delete shaderDataCollection.second.shadersParamLayout;

        delete shaderDataCollection.second.shaderObject;
    }
    rawShaderObjects.clear();
}

void GlobalRenderingContextBase::writeAndDestroyPipelineCache()
{
    if (pipelinesCache)
    {
        for (const std::pair<String, ShaderDataCollection>& shaderDataCollection : rawShaderObjects)
        {
            if (shaderDataCollection.second.shaderObject->baseShaderType() == DrawMeshShader::staticType())
            {
                const DrawMeshShaderObject* drawMeshShaderObj = static_cast<const DrawMeshShaderObject*>(shaderDataCollection.second.shaderObject);
                for (const std::pair<const DrawMeshShader*, GraphicsPipelineBase*>& shaderResourcePair : drawMeshShaderObj->getAllShaders())
                {
                    pipelinesCache->addPipelineToCache(shaderResourcePair.second);
                }
            }
            else if (shaderDataCollection.second.shaderObject->baseShaderType() == UniqueUtilityShader::staticType())
            {
                const UniqueUtilityShaderObject* uniqUtilShaderObj = static_cast<const UniqueUtilityShaderObject*>(shaderDataCollection.second.shaderObject);
                for (const GraphicsPipelineBase* pipeline : uniqUtilShaderObj->getAllPipelines())
                {
                    pipelinesCache->addPipelineToCache(pipeline);
                }
            }
        }

        pipelinesCache->writeCache();
        pipelinesCache->release();
        delete pipelinesCache;
        pipelinesCache = nullptr;
    }
}

GenericRenderPassProperties GlobalRenderingContextBase::renderpassPropsFromRTs(const std::vector<RenderTargetTexture*>& rtTextures) const
{
    GenericRenderPassProperties renderpassProperties;
    renderpassProperties.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;
    if (!rtTextures.empty())
    {
        // Since all the textures in a same framebuffer must have same properties on below two
        renderpassProperties.bOneRtPerFormat = rtTextures[0]->isSameReadWriteTexture();
        renderpassProperties.multisampleCount = rtTextures[0]->getSampleCount();

        renderpassProperties.renderpassAttachmentFormat.attachments.reserve(rtTextures.size());
        for (const RenderTargetTexture*const & rtTexture : rtTextures)
        {
            renderpassProperties.renderpassAttachmentFormat.attachments.emplace_back(rtTexture->getFormat());
        }
    }

    return renderpassProperties;
}

GenericRenderPassProperties GlobalRenderingContextBase::renderpassPropsFromFb(const Framebuffer* fb) const
{
    GenericRenderPassProperties renderpassProperties;
    renderpassProperties.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;
    if (!fb->textures.empty())
    {
        // Since all the textures in a same framebuffer must have same properties on below two
        renderpassProperties.bOneRtPerFormat = !fb->bHasResolves;
        renderpassProperties.multisampleCount = fb->textures[0]->sampleCount();

        renderpassProperties.renderpassAttachmentFormat.attachments.reserve(fb->textures.size());
        for (int32 i = 0; i < fb->textures.size(); ++i)
        {
            renderpassProperties.renderpassAttachmentFormat.attachments.emplace_back(fb->textures[i]->imageFormat());
            i += fb->bHasResolves && !EPixelDataFormat::isDepthFormat(fb->textures[i]->imageFormat()) ? 1 : 0;
        }
        renderpassProperties.renderpassAttachmentFormat.attachments.shrink_to_fit();
    }

    return renderpassProperties;
}

const Framebuffer* GlobalRenderingContextBase::getFramebuffer(const GenericRenderPassProperties& renderpassProps, const std::vector<RenderTargetTexture*>& rtTextures) const
{
    auto renderpassFbs = rtFramebuffers.find(renderpassProps);

    if (renderpassFbs != rtFramebuffers.cend() && !renderpassFbs->second.empty())
    {
        if (renderpassProps.renderpassAttachmentFormat.attachments.empty())
        {
            // there can be only one render pass without any attachments.
            return renderpassFbs->second[0];
        }
        // Note: not handling outdated resources case right now
        std::vector<const ImageResource*> expectedAttachments;
        for (const RenderTargetTexture* const& rtTexture : rtTextures)
        {
            expectedAttachments.emplace_back(rtTexture->getRtTexture());

            // Since depth formats do not have resolve
            if (!renderpassProps.bOneRtPerFormat && !EPixelDataFormat::isDepthFormat(rtTexture->getFormat()))
            {
                expectedAttachments.emplace_back(rtTexture->getTextureResource());
            }
        }

        for (const Framebuffer* const& fb : renderpassFbs->second)
        {
            if (fb->textures.size() == expectedAttachments.size())
            {
                bool bSameTextures = true;
                for (int32 attachmentIdx = 0; attachmentIdx < renderpassFbs->second.size(); ++attachmentIdx)
                {
                    bSameTextures = bSameTextures && fb->textures[attachmentIdx] == expectedAttachments[attachmentIdx];
                }

                if (bSameTextures)
                {
                    return fb;
                }
            }
        }
    }
    return nullptr;
}

const Framebuffer* GlobalRenderingContextBase::createNewFramebuffer(const GenericRenderPassProperties& renderpassProps
    , const std::vector<RenderTargetTexture*>& rtTextures) const
{
    Framebuffer* fb = GBuffers::createFbInstance();
    fb->bHasResolves = !renderpassProps.bOneRtPerFormat;

    if (rtTextures.empty())
    {
        GBuffers::initializeFb(fb, Size2D());
    }
    else
    {
        for (const RenderTargetTexture* const& rtTexture : rtTextures)
        {
            fb->textures.emplace_back(rtTexture->getRtTexture());

            // Since depth formats do not have resolve
            if (!renderpassProps.bOneRtPerFormat && !EPixelDataFormat::isDepthFormat(rtTexture->getFormat()))
            {
                fb->textures.emplace_back(rtTexture->getTextureResource());
            }
        }

        GBuffers::initializeFb(fb, rtTextures[0]->getTextureSize());
    }

    return fb;
}

PipelineBase* GlobalRenderingContextBase::createNewPipeline(UniqueUtilityShaderObject* shaderObject
    , const GenericRenderPassProperties& renderpassProps)
{
    fatalAssert(renderpassProps.renderpassAttachmentFormat.attachments.size() == shaderObject->getDefaultPipeline()->getRenderpassProperties().renderpassAttachmentFormat.attachments.size()
        , "Attachment count must be same for utility shader pipeline variants");
    GraphicsPipelineBase* pipeline = static_cast<GraphicsPipelineBase*>(pipelineFactory->create({ shaderObject->getShader(), shaderObject->getDefaultPipeline() }));
    pipeline->setRenderpassProperties(renderpassProps);

    initializeNewPipeline(shaderObject, pipeline);
    return pipeline;
}

void GlobalRenderingContextBase::preparePipelineContext(class LocalPipelineContext* pipelineContext)
{
    std::unordered_map<String, ShaderDataCollection>::const_iterator shaderDataCollectionItr 
        = rawShaderObjects.find(pipelineContext->materialName);
    if (shaderDataCollectionItr == rawShaderObjects.cend())
    {
        Logger::error("GlobalRenderingContext", "%s : Requested material %s is not found", __func__, pipelineContext->materialName.getChar());
        return;
    }

    if (shaderDataCollectionItr->second.shaderObject->baseShaderType() == DrawMeshShader::staticType())
    {
        DrawMeshShaderObject* drawMeshShaderObj = static_cast<DrawMeshShaderObject*>(shaderDataCollectionItr->second.shaderObject);

        GraphicsPipelineBase* graphicsPipeline;
        drawMeshShaderObj->getShader(pipelineContext->forVertexType, FramebufferFormat(pipelineContext->renderpassFormat), &graphicsPipeline);
        pipelineContext->pipelineUsed = graphicsPipeline;

        pipelineContext->framebuffer = GBuffers::getFramebuffer(pipelineContext->renderpassFormat, pipelineContext->swapchainIdx);
    }
    else if(shaderDataCollectionItr->second.shaderObject->baseShaderType() == UniqueUtilityShader::staticType())
    {
        GenericRenderPassProperties renderpassProps;
        const Framebuffer* fb = nullptr;
        if (pipelineContext->bUseSwapchainFb)
        {
            fb = GBuffers::getSwapchainFramebuffer(pipelineContext->swapchainIdx);

            const GenericWindowCanvas* windowCanvas = gEngine->getApplicationInstance()->appWindowManager
                .getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow());
            renderpassProps.bOneRtPerFormat = true;
            renderpassProps.multisampleCount = EPixelSampleCount::SampleCount1;
            renderpassProps.renderpassAttachmentFormat.attachments.resize(1);
            renderpassProps.renderpassAttachmentFormat.attachments[0] = windowCanvas->windowCanvasFormat();
            renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;
        }
        else
        {
            renderpassProps = renderpassPropsFromRTs(pipelineContext->rtTextures);
            fb = getFramebuffer(renderpassProps, pipelineContext->rtTextures);
            if (fb == nullptr)
            {
                fb = createNewFramebuffer(renderpassProps, pipelineContext->rtTextures);
                rtFramebuffers[renderpassProps].emplace_back(fb);
            }
        }
        UniqueUtilityShaderObject* uniqUtilShaderObj = static_cast<UniqueUtilityShaderObject*>(shaderDataCollectionItr->second.shaderObject);
        GraphicsPipelineBase* graphicsPipeline = uniqUtilShaderObj->getPipeline(renderpassProps);
        if (graphicsPipeline == nullptr)
        {
            graphicsPipeline = static_cast<GraphicsPipelineBase*>(createNewPipeline(uniqUtilShaderObj, renderpassProps));
            uniqUtilShaderObj->setPipeline(renderpassProps, graphicsPipeline);
        }
        pipelineContext->pipelineUsed = graphicsPipeline;        
        pipelineContext->framebuffer = fb;
    }
}
