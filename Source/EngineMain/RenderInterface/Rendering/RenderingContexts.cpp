#include "RenderingContexts.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Platform/PlatformFunctions.h"
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
    std::map<String, uint32>& runtimeResCount = ShaderParameterUtility::unboundArrayResourcesCount();
    // Fill Runtime indexed resources max count over here
    runtimeResCount["srcImages"] = 16u;
    runtimeResCount["globalSampledTexs"] = 128u;

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

    for (const std::pair<const GenericRenderPassProperties, std::vector<const Framebuffer*>>& framebuffers : rtFramebuffers)
    {
        for (Framebuffer const* const& fb : framebuffers.second)
        {
            GlobalBuffers::destroyFbInstance(fb);
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

    std::map<String, std::pair<uint32, ShaderResource*>> shaderUniqParamUsageMaxBitCount;
    std::vector<GraphicsResource*> allShaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(allShaderResources, true, true);
    // Initialize all shaders
    {
        uint32 bindlessUsageMaxBitCount = 0;
        ShaderResource* bindlessParamUsedInShader = nullptr;

        uint32 viewParamUsageMaxBitCount = 0;
        ShaderResource* viewParamUsedInShader = nullptr;

        std::map<EVertexType::Type, std::pair<uint32, ShaderResource*>> vertexParamUsageMaxBitCount;

        for (GraphicsResource* shader : allShaderResources)
        {
            shader->init();

            if (shader->getType()->isChildOf(DrawMeshShader::staticType()))
            {
                DrawMeshShader* drawMeshShader = static_cast<DrawMeshShader*>(shader);

                for (const ReflectDescriptorBody& descriptorsSetMeta : drawMeshShader->getReflection()->descriptorsSets)
                {
                    uint32 setBitCount = PlatformFunctions::getSetBitCount(descriptorsSetMeta.combinedSetUsage);

                    if (descriptorsSetMeta.set == ShaderParameterUtility::INSTANCE_UNIQ_SET) // Vertex param
                    {
                        auto itr = vertexParamUsageMaxBitCount.find(drawMeshShader->vertexUsage());
                        if (itr == vertexParamUsageMaxBitCount.end())
                        {
                            vertexParamUsageMaxBitCount[drawMeshShader->vertexUsage()] = { setBitCount, drawMeshShader };
                        }
                        else if (itr->second.first < setBitCount)
                        {
                            itr->second.first = setBitCount;
                            itr->second.second = drawMeshShader;
                        }
                    }
                    else if (descriptorsSetMeta.set == ShaderParameterUtility::SHADER_UNIQ_SET)
                    {
                        auto itr = shaderUniqParamUsageMaxBitCount.find(drawMeshShader->getResourceName());
                        if (itr == shaderUniqParamUsageMaxBitCount.end())
                        {
                            shaderUniqParamUsageMaxBitCount[drawMeshShader->getResourceName()] = { setBitCount, drawMeshShader };
                        }
                        else if (itr->second.first < setBitCount)
                        {
                            itr->second.first = setBitCount;
                            itr->second.second = drawMeshShader;
                        }
                    }
                    else if(descriptorsSetMeta.set == ShaderParameterUtility::VIEW_UNIQ_SET && viewParamUsageMaxBitCount < setBitCount) // View param
                    {
                        viewParamUsageMaxBitCount = setBitCount;
                        viewParamUsedInShader = drawMeshShader;
                    }
                    else if (descriptorsSetMeta.set == ShaderParameterUtility::BINDLESS_SET && bindlessUsageMaxBitCount < setBitCount) // bind less
                    {
                        bindlessUsageMaxBitCount = setBitCount;
                        bindlessParamUsedInShader = drawMeshShader;
                    }
                }
            }
        }

        // View unique param layout
        debugAssert(viewParamUsedInShader != nullptr && sceneViewParamLayout == nullptr);
        sceneViewParamLayout = shaderParamLayoutsFactory->create(viewParamUsedInShader, ShaderParameterUtility::VIEW_UNIQ_SET);
        debugAssert(sceneViewParamLayout != nullptr);
        sceneViewParamLayout->init();
        // Bind less
        debugAssert(bindlessParamUsedInShader != nullptr && bindlessParamLayout == nullptr);
        bindlessParamLayout = shaderParamLayoutsFactory->create(bindlessParamUsedInShader, ShaderParameterUtility::BINDLESS_SET);
        debugAssert(bindlessParamLayout != nullptr);
        bindlessParamLayout->init();

        for (const std::pair<const EVertexType::Type, std::pair<uint32, ShaderResource*>>& vertexParamLayoutInfo : vertexParamUsageMaxBitCount)
        {
            // Vertex unique param layout
            auto perVertParamLayoutItr = perVertexTypeLayouts.find(static_cast<DrawMeshShader*>(vertexParamLayoutInfo.second.second)->vertexUsage());
            debugAssert(perVertParamLayoutItr == perVertexTypeLayouts.end());
            GraphicsResource* paramLayout = shaderParamLayoutsFactory->create(vertexParamLayoutInfo.second.second, ShaderParameterUtility::INSTANCE_UNIQ_SET);
            debugAssert(paramLayout != nullptr);
            paramLayout->init();
            perVertexTypeLayouts[static_cast<DrawMeshShader*>(vertexParamLayoutInfo.second.second)->vertexUsage()] = paramLayout;
        }
    }
    initShaderPipelines(allShaderResources, shaderUniqParamUsageMaxBitCount);
}

void GlobalRenderingContextBase::initShaderPipelines(const std::vector<GraphicsResource*>& allShaderResources
    , const std::map<String, std::pair<uint32, ShaderResource*>>& shaderUniqParamShader)
{
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

    for (GraphicsResource* shader : allShaderResources)
    {
        if (shader->getType()->isChildOf(DrawMeshShader::staticType()))
        {
            DrawMeshShader* drawMeshShader = static_cast<DrawMeshShader*>(shader);
            vertexAttribFillLambda(drawMeshShader->vertexUsage(), drawMeshShader->getReflection()->inputs);

            ShaderDataCollection& shaderCollection = rawShaderObjects[shader->getResourceName()];
            if (shaderCollection.shadersParamLayout == nullptr)
            {
                ShaderResource* shaderToUse = drawMeshShader;
                auto shaderToUseItr = shaderUniqParamShader.find(drawMeshShader->getResourceName());
                if (shaderToUseItr != shaderUniqParamShader.cend())
                {
                    shaderToUse = shaderToUseItr->second.second;
                }
                shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(shaderToUse, ShaderParameterUtility::SHADER_UNIQ_SET);
                shaderCollection.shadersParamLayout->init();
            }
            if (shaderCollection.shaderObject == nullptr)
            {
                shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), drawMeshShader);
            }
            GraphicsPipelineBase* graphicsPipeline = static_cast<GraphicsPipelineBase*>(pipelineFactory->create({ drawMeshShader }));

            // Check if there is set 3(Per variant shader parameters)
            GraphicsResource* perVariantLayout = nullptr;
            for (const ReflectDescriptorBody& reflectDescBody : drawMeshShader->getReflection()->descriptorsSets)
            {
                if (reflectDescBody.set == ShaderParameterUtility::SHADER_VARIANT_UNIQ_SET)
                {
                    perVariantLayout = shaderParamLayoutsFactory->create(drawMeshShader, reflectDescBody.set);
                    perVariantLayout->init();
                    graphicsPipeline->setParamLayoutAtSet(perVariantLayout, reflectDescBody.set);
                }
            }
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout, ShaderParameterUtility::SHADER_UNIQ_SET);
            graphicsPipeline->setParamLayoutAtSet(perVertexTypeLayouts[drawMeshShader->vertexUsage()], ShaderParameterUtility::INSTANCE_UNIQ_SET);
            graphicsPipeline->setParamLayoutAtSet(sceneViewParamLayout, ShaderParameterUtility::VIEW_UNIQ_SET);
            graphicsPipeline->setParamLayoutAtSet(bindlessParamLayout, ShaderParameterUtility::BINDLESS_SET);
            graphicsPipeline->setPipelineShader(drawMeshShader);
            graphicsPipeline->setPipelineCache(pipelinesCache);
            GenericRenderPassProperties renderpassProp;
            renderpassProp.renderpassAttachmentFormat.rpFormat = drawMeshShader->renderpassUsage();
            graphicsPipeline->setRenderpassProperties(renderpassProp);

            DrawMeshShaderObject* drawMeshShaderObj = static_cast<DrawMeshShaderObject*>(shaderCollection.shaderObject);
            drawMeshShaderObj->addShader(drawMeshShader);
            drawMeshShaderObj->setPipeline(drawMeshShader, graphicsPipeline);
            drawMeshShaderObj->setVariantParamsLayout(drawMeshShader, perVariantLayout);
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
            graphicsPipeline->setPipelineShader(utilityShader);
            graphicsPipeline->setPipelineCache(pipelinesCache);
            // Can be parents since other child pipelines will be derived from this initial defaults
            graphicsPipeline->setCanBeParent(true);

            static_cast<UniqueUtilityShaderObject*>(shaderCollection.shaderObject)->setPipeline(graphicsPipeline->getRenderpassProperties()
                , graphicsPipeline);
        }
        else if (shader->getType()->isChildOf<ComputeShader>())
        {
            ShaderResource* shaderResource = static_cast<ShaderResource*>(shader);
            ShaderDataCollection& shaderCollection = rawShaderObjects[shader->getResourceName()];
            debugAssert(shaderCollection.shaderObject == nullptr);
            debugAssert(shaderCollection.shadersParamLayout == nullptr);

            shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), shaderResource);
            shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(shaderResource, 0/*doesn't matter*/);
            shaderCollection.shadersParamLayout->init();

            PipelineBase *pipeline = pipelineFactory->create({ shaderResource });
            pipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout);
            pipeline->setPipelineShader(shaderResource);
            pipeline->setPipelineCache(pipelinesCache);

            static_cast<ComputeShaderObject*>(shaderCollection.shaderObject)->setPipeline(static_cast<ComputePipelineBase*>(pipeline));
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
    bindlessParamLayout->release();
    delete bindlessParamLayout;
    bindlessParamLayout = nullptr;

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
        for (const std::pair<const String, ShaderDataCollection>& shaderDataCollection : rawShaderObjects)
        {
            shaderDataCollection.second.shaderObject->preparePipelineCache(pipelinesCache);
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

GenericRenderPassProperties GlobalRenderingContextBase::renderpassPropsFromRpFormat(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx) const
{
    const Framebuffer* fb = GlobalBuffers::getFramebuffer(renderpassFormat, frameIdx);
    if (fb)
    {
        return renderpassPropsFromFb(fb);
    }

    return GlobalBuffers::getFramebufferRenderpassProps(renderpassFormat);
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
        // Note: not handling outdated resources case right now, if outdated remove manually and recreate fbs for external RTs
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
                for (int32 attachmentIdx = 0; attachmentIdx < fb->textures.size(); ++attachmentIdx)
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
    Framebuffer* fb = GlobalBuffers::createFbInstance();
    fb->bHasResolves = !renderpassProps.bOneRtPerFormat;

    if (rtTextures.empty())
    {
        GlobalBuffers::initializeFb(fb, Size2D());
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

        GlobalBuffers::initializeFb(fb, rtTextures[0]->getTextureSize());
    }

    return fb;
}

const Framebuffer* GlobalRenderingContextBase::getOrCreateFramebuffer(const GenericRenderPassProperties& renderpassProps, const std::vector<RenderTargetTexture*>& rtTextures)
{
    const Framebuffer* fb = getFramebuffer(renderpassProps, rtTextures);
    if (fb == nullptr)
    {
        fb = createNewFramebuffer(renderpassProps, rtTextures);
        rtFramebuffers[renderpassProps].emplace_back(fb);
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

    initializeGenericGraphicsPipeline(shaderObject, pipeline);
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

        // If empty RTs then get framebuffer from global buffers
        const Framebuffer* fb = nullptr;
        if (pipelineContext->rtTextures.empty())
        {
            fb = GlobalBuffers::getFramebuffer(pipelineContext->renderpassFormat, pipelineContext->swapchainIdx);
        }
        else
        {
            GenericRenderPassProperties renderpassProps = renderpassPropsFromRTs(pipelineContext->rtTextures);
            renderpassProps.renderpassAttachmentFormat.rpFormat = pipelineContext->renderpassFormat;
            // To make sure that RTs created framebuffer is compatible with GlobalBuffers created FBs and its render pass and pipelines
            fatalAssert(renderpassProps == renderpassPropsFromRpFormat(pipelineContext->renderpassFormat, pipelineContext->swapchainIdx)
                , "%s() : Incompatible RTs for Mesh Draw shaders", __func__);

            fb = getOrCreateFramebuffer(renderpassProps, pipelineContext->rtTextures);
        }
        fatalAssert(fb != nullptr, "%s() : Framebuffer is invalid[Shader : %s, Render pass format : %s]", __func__, pipelineContext->materialName.getChar()
            , ERenderPassFormat::toString(pipelineContext->renderpassFormat));
        pipelineContext->framebuffer = fb;
    }
    else if(shaderDataCollectionItr->second.shaderObject->baseShaderType() == UniqueUtilityShader::staticType())
    {
        GenericRenderPassProperties renderpassProps;
        const Framebuffer* fb = nullptr;
        if (pipelineContext->bUseSwapchainFb)
        {
            fb = GlobalBuffers::getSwapchainFramebuffer(pipelineContext->swapchainIdx);

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
            fb = getOrCreateFramebuffer(renderpassProps, pipelineContext->rtTextures);
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
    else if (shaderDataCollectionItr->second.shaderObject->baseShaderType() == ComputeShader::staticType())
    {
        ComputeShaderObject* computeShaderObj = static_cast<ComputeShaderObject*>(shaderDataCollectionItr->second.shaderObject);
        pipelineContext->pipelineUsed = computeShaderObj->getPipeline();
    }
}

void GlobalRenderingContextBase::clearExternInitRtsFramebuffer(const std::vector<RenderTargetTexture*>& rtTextures)
{
    GenericRenderPassProperties renderpassProps = renderpassPropsFromRTs(rtTextures);

    auto renderpassFbs = rtFramebuffers.find(renderpassProps);
    if (renderpassFbs != rtFramebuffers.cend() && !renderpassFbs->second.empty())
    {
        if (renderpassProps.renderpassAttachmentFormat.attachments.empty())
        {
            // there can be only one render pass without any attachments.
            GlobalBuffers::destroyFbInstance(renderpassFbs->second[0]);
            renderpassFbs->second.clear();
            return;
        }
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

        for (auto itr = renderpassFbs->second.begin(); itr != renderpassFbs->second.end(); ++itr)
        {
            const Framebuffer *fb = *itr;
            if (fb->textures.size() == expectedAttachments.size())
            {
                bool bSameTextures = true;
                for (int32 attachmentIdx = 0; attachmentIdx < fb->textures.size(); ++attachmentIdx)
                {
                    bSameTextures = bSameTextures && fb->textures[attachmentIdx] == expectedAttachments[attachmentIdx];
                }

                if (bSameTextures)
                {
                    GlobalBuffers::destroyFbInstance(fb);
                    renderpassFbs->second.erase(itr);
                    return;
                }
            }
        }
    }
}
