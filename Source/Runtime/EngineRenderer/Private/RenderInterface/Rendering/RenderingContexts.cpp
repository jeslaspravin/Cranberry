/*!
 * \file RenderingContexts.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Rendering/RenderingContexts.h"
#include "IRenderInterfaceModule.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderObject.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "ShaderReflected.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "RenderInterface/GlobalRenderVariables.h"

void GlobalRenderingContextBase::initContext(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    graphicsInstanceCache = graphicsInstance;
    graphicsHelperCache = graphicsHelper;

    std::map<String, uint32> &runtimeResCount = ShaderParameterUtility::unboundArrayResourcesCount();
    // Fill Runtime indexed resources max count over here
    runtimeResCount[TCHAR("srcImages")] = 16u;
    runtimeResCount[TCHAR("globalSampledTexs")] = GlobalRenderVariables::GLOBAL_SAMPLED_TEX_NUM.get();

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

    // Deleting all created framebuffers
    for (const std::pair<const GenericRenderPassProperties, std::vector<const Framebuffer *>> &framebuffers : rtFramebuffers)
    {
        for (Framebuffer const *const &fb : framebuffers.second)
        {
            delete fb;
        }
    }
    rtFramebuffers.clear();

    // Deleting all created swapchain framebuffers
    for (const std::pair<WindowCanvasRef, std::vector<const Framebuffer *>> &framebuffers : windowCanvasFramebuffers)
    {
        for (const Framebuffer *fb : framebuffers.second)
        {
            delete fb;
        }
    }
    windowCanvasFramebuffers.clear();

    clearApiContext();
}

void GlobalRenderingContextBase::initShaderResources()
{
    if (pipelinesCache)
    {
        pipelinesCache->setResourceName(TCHAR("shaders"));
        pipelinesCache->init();
    }

    std::map<String, std::pair<uint32, ShaderResource *>> shaderUniqParamUsageMaxBitCount;
    std::vector<ShaderResource *> allShaderResources;
    std::vector<GraphicsResource *> allShaderConfigs;
    ShaderConfigCollector::staticType()->allChildDefaultResources(allShaderConfigs, true, true);
    allShaderResources.reserve(allShaderConfigs.size());

    // Initialize all shaders
    {
        uint32 bindlessUsageMaxBitCount = 0;
        ShaderResource *bindlessParamUsedInShader = nullptr;

        uint32 viewParamUsageMaxBitCount = 0;
        ShaderResource *viewParamUsedInShader = nullptr;

        std::map<EVertexType::Type, std::pair<uint32, ShaderResource *>> vertexParamUsageMaxBitCount;

        for (GraphicsResource *config : allShaderConfigs)
        {
            ShaderConfigCollector *shaderConfig = static_cast<ShaderConfigCollector *>(config);
            bool bDrawMeshShaderConfig = shaderConfig->getType()->isChildOf(DrawMeshShaderConfig::staticType());
            if (GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.get()
                && (bDrawMeshShaderConfig || shaderConfig->getType()->isChildOf(UniqueUtilityShaderConfig::staticType())))
            {
                // We are in compute only mode
                continue;
            }

            ShaderResource *shader = graphicsHelperCache->createShaderResource(shaderConfig);
            shader->init();
            allShaderResources.emplace_back(shader);
            shaderConfig->setShaderConfigured(shader);

            if (bDrawMeshShaderConfig)
            {
                const DrawMeshShaderConfig *drawMeshShaderConfig = static_cast<const DrawMeshShaderConfig *>(shaderConfig);

                for (const ReflectDescriptorBody &descriptorsSetMeta : shader->getReflection()->descriptorsSets)
                {
                    uint32 setBitCount = PlatformFunctions::getSetBitCount(descriptorsSetMeta.combinedSetUsage);

                    if (descriptorsSetMeta.set == ShaderParameterUtility::INSTANCE_UNIQ_SET) // Vertex param
                    {
                        auto itr = vertexParamUsageMaxBitCount.find(drawMeshShaderConfig->vertexUsage());
                        if (itr == vertexParamUsageMaxBitCount.end())
                        {
                            vertexParamUsageMaxBitCount[drawMeshShaderConfig->vertexUsage()] = { setBitCount, shader };
                        }
                        else if (itr->second.first < setBitCount)
                        {
                            itr->second.first = setBitCount;
                            itr->second.second = shader;
                        }
                    }
                    else if (descriptorsSetMeta.set == ShaderParameterUtility::SHADER_UNIQ_SET)
                    {
                        auto itr = shaderUniqParamUsageMaxBitCount.find(drawMeshShaderConfig->getResourceName());
                        if (itr == shaderUniqParamUsageMaxBitCount.end())
                        {
                            shaderUniqParamUsageMaxBitCount[drawMeshShaderConfig->getResourceName()] = { setBitCount, shader };
                        }
                        else if (itr->second.first < setBitCount)
                        {
                            itr->second.first = setBitCount;
                            itr->second.second = shader;
                        }
                    }
                    else if (descriptorsSetMeta.set == ShaderParameterUtility::VIEW_UNIQ_SET && viewParamUsageMaxBitCount < setBitCount) // View
                                                                                                                                         // param
                    {
                        viewParamUsageMaxBitCount = setBitCount;
                        viewParamUsedInShader = shader;
                    }
                    else if (descriptorsSetMeta.set == ShaderParameterUtility::BINDLESS_SET && bindlessUsageMaxBitCount < setBitCount) // bind
                                                                                                                                       // less
                    {
                        bindlessUsageMaxBitCount = setBitCount;
                        bindlessParamUsedInShader = shader;
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

        for (const std::pair<const EVertexType::Type, std::pair<uint32, ShaderResource *>> &vertexParamLayoutInfo : vertexParamUsageMaxBitCount)
        {
            EVertexType::Type vertUsage
                = static_cast<const DrawMeshShaderConfig *>(vertexParamLayoutInfo.second.second->getShaderConfig())->vertexUsage();

            // Vertex unique param layout
            auto perVertParamLayoutItr = perVertexTypeLayouts.find(vertUsage);
            debugAssert(perVertParamLayoutItr == perVertexTypeLayouts.end());
            GraphicsResource *paramLayout
                = shaderParamLayoutsFactory->create(vertexParamLayoutInfo.second.second, ShaderParameterUtility::INSTANCE_UNIQ_SET);
            debugAssert(paramLayout != nullptr);
            paramLayout->init();
            perVertexTypeLayouts[vertUsage] = paramLayout;
        }
    }
    initShaderPipelines(allShaderResources, shaderUniqParamUsageMaxBitCount);
}

void GlobalRenderingContextBase::initShaderPipelines(
    const std::vector<ShaderResource *> &allShaderResources, const std::map<String, std::pair<uint32, ShaderResource *>> &shaderUniqParamShader
)
{
    std::set<EVertexType::Type> filledVertexInfo;
    auto vertexAttribFillLambda = [&filledVertexInfo](EVertexType::Type vertexUsed, const std::vector<ReflectInputOutput> &vertexShaderInputs)
    {
        // If not filled yet
        if (filledVertexInfo.find(vertexUsed) == filledVertexInfo.end())
        {
            filledVertexInfo.insert(vertexUsed);
            const std::vector<ShaderVertexParamInfo *> &vertexBindingsInfo = EVertexType::vertexParamInfo(vertexUsed);
            for (ShaderVertexParamInfo *vertexBindingAttributes : vertexBindingsInfo)
            {
                ShaderParameterUtility::fillRefToVertexParamInfo(*vertexBindingAttributes, vertexShaderInputs);
            }
        }
    };

    for (ShaderResource *shader : allShaderResources)
    {
        if (shader->getShaderConfig()->getType()->isChildOf(DrawMeshShaderConfig::staticType()))
        {
            debugAssert(!GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.get());

            const DrawMeshShaderConfig *drawMeshShaderConfig = static_cast<const DrawMeshShaderConfig *>(shader->getShaderConfig());
            vertexAttribFillLambda(drawMeshShaderConfig->vertexUsage(), drawMeshShaderConfig->getReflection()->inputs);

            ShaderDataCollection &shaderCollection = rawShaderObjects[shader->getResourceName()];
            if (shaderCollection.shadersParamLayout == nullptr)
            {
                ShaderResource *shaderToUse = shader;
                auto shaderToUseItr = shaderUniqParamShader.find(shader->getResourceName());
                if (shaderToUseItr != shaderUniqParamShader.cend())
                {
                    shaderToUse = shaderToUseItr->second.second;
                }
                shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(shaderToUse, ShaderParameterUtility::SHADER_UNIQ_SET);
                shaderCollection.shadersParamLayout->init();
            }
            if (shaderCollection.shaderObject == nullptr)
            {
                shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), shader);
            }
            GraphicsPipelineBase *graphicsPipeline
                = static_cast<GraphicsPipelineBase *>(pipelineFactory->create(graphicsInstanceCache, graphicsHelperCache, { shader }));
            fatalAssert(
                graphicsPipeline, "%s() : Graphics pipeline creation failed for shader %s", __func__, shader->getResourceName().getChar()
            );

            // Check if there is set 3(Per variant shader parameters)
            GraphicsResource *perVariantLayout = nullptr;
            for (const ReflectDescriptorBody &reflectDescBody : shader->getReflection()->descriptorsSets)
            {
                if (reflectDescBody.set == ShaderParameterUtility::SHADER_VARIANT_UNIQ_SET)
                {
                    perVariantLayout = shaderParamLayoutsFactory->create(shader, reflectDescBody.set);
                    perVariantLayout->init();
                    graphicsPipeline->setParamLayoutAtSet(perVariantLayout, reflectDescBody.set);
                }
            }
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout, ShaderParameterUtility::SHADER_UNIQ_SET);
            graphicsPipeline->setParamLayoutAtSet(
                perVertexTypeLayouts[drawMeshShaderConfig->vertexUsage()], ShaderParameterUtility::INSTANCE_UNIQ_SET
            );
            graphicsPipeline->setParamLayoutAtSet(sceneViewParamLayout, ShaderParameterUtility::VIEW_UNIQ_SET);
            graphicsPipeline->setParamLayoutAtSet(bindlessParamLayout, ShaderParameterUtility::BINDLESS_SET);
            graphicsPipeline->setPipelineShader(shader);
            graphicsPipeline->setPipelineCache(pipelinesCache);
            GenericRenderPassProperties renderpassProp;
            renderpassProp.renderpassAttachmentFormat.rpFormat = drawMeshShaderConfig->renderpassUsage();
            graphicsPipeline->setRenderpassProperties(renderpassProp);

            DrawMeshShaderObject *drawMeshShaderObj = static_cast<DrawMeshShaderObject *>(shaderCollection.shaderObject);
            drawMeshShaderObj->addShader(shader);
            drawMeshShaderObj->setPipeline(shader, graphicsPipeline);
            drawMeshShaderObj->setVariantParamsLayout(shader, perVariantLayout);
        }
        else if (shader->getShaderConfig()->getType()->isChildOf(UniqueUtilityShaderConfig::staticType()))
        {
            debugAssert(!GlobalRenderVariables::GPU_IS_COMPUTE_ONLY.get());

            const UniqueUtilityShaderConfig *utilityShaderConfig = static_cast<const UniqueUtilityShaderConfig *>(shader->getShaderConfig());
            vertexAttribFillLambda(utilityShaderConfig->vertexUsage(), utilityShaderConfig->getReflection()->inputs);

            ShaderDataCollection &shaderCollection = rawShaderObjects[shader->getResourceName()];
            debugAssert(shaderCollection.shaderObject == nullptr);
            debugAssert(shaderCollection.shadersParamLayout == nullptr);

            shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), shader);
            shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(shader, 0 /*doesn't matter*/);
            shaderCollection.shadersParamLayout->init();

            GraphicsPipelineBase *graphicsPipeline
                = static_cast<GraphicsPipelineBase *>(pipelineFactory->create(graphicsInstanceCache, graphicsHelperCache, { shader }));
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout);
            graphicsPipeline->setPipelineShader(shader);
            graphicsPipeline->setPipelineCache(pipelinesCache);
            // Can be parents since other child pipelines will be derived from this initial
            // defaults
            graphicsPipeline->setCanBeParent(true);

            static_cast<UniqueUtilityShaderObject *>(shaderCollection.shaderObject)
                ->setPipeline(graphicsPipeline->getRenderpassProperties(), graphicsPipeline);
        }
        else if (shader->getShaderConfig()->getType()->isChildOf<ComputeShaderConfig>())
        {
            ShaderDataCollection &shaderCollection = rawShaderObjects[shader->getResourceName()];
            debugAssert(shaderCollection.shaderObject == nullptr);
            debugAssert(shaderCollection.shadersParamLayout == nullptr);

            shaderCollection.shaderObject = shaderObjectFactory->create(shader->getResourceName(), shader);
            shaderCollection.shadersParamLayout = shaderParamLayoutsFactory->create(shader, 0 /*doesn't matter*/);
            shaderCollection.shadersParamLayout->init();

            PipelineBase *pipeline = pipelineFactory->create(graphicsInstanceCache, graphicsHelperCache, { shader });
            pipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout);
            pipeline->setPipelineShader(shader);
            pipeline->setPipelineCache(pipelinesCache);

            static_cast<ComputeShaderObject *>(shaderCollection.shaderObject)->setPipeline(static_cast<ComputePipelineBase *>(pipeline));
        }
    }
}

void GlobalRenderingContextBase::destroyShaderResources()
{
    std::vector<GraphicsResource *> shaderResources;
    ShaderResource::staticType()->allRegisteredResources(shaderResources, true, true);
    for (GraphicsResource *shader : shaderResources)
    {
        shader->release();
        delete shader;
    }

    sceneViewParamLayout->release();
    delete sceneViewParamLayout;
    sceneViewParamLayout = nullptr;
    bindlessParamLayout->release();
    delete bindlessParamLayout;
    bindlessParamLayout = nullptr;

    for (std::pair<const EVertexType::Type, GraphicsResource *> &shaderVertexParamLayout : perVertexTypeLayouts)
    {
        shaderVertexParamLayout.second->release();
        delete shaderVertexParamLayout.second;
    }
    perVertexTypeLayouts.clear();

    for (std::pair<const String, ShaderDataCollection> &shaderDataCollection : rawShaderObjects)
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
        for (const std::pair<const String, ShaderDataCollection> &shaderDataCollection : rawShaderObjects)
        {
            shaderDataCollection.second.shaderObject->preparePipelineCache(pipelinesCache);
        }

        pipelinesCache->writeCache();
        pipelinesCache->release();
        delete pipelinesCache;
        pipelinesCache = nullptr;
    }
}

// GenericRenderPassProperties GlobalRenderingContextBase::renderpassPropsFromRTs(const
// std::vector<RenderTargetTexture*>& rtTextures) const
//{
//     GenericRenderPassProperties renderpassProperties;
//     renderpassProperties.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;
//     if (!rtTextures.empty())
//     {
//         // Since all the textures in a same framebuffer must have same properties on below two
//         renderpassProperties.bOneRtPerFormat = rtTextures[0]->isSameReadWriteTexture();
//         renderpassProperties.multisampleCount = rtTextures[0]->getSampleCount();
//
//         renderpassProperties.renderpassAttachmentFormat.attachments.reserve(rtTextures.size());
//         for (const RenderTargetTexture*const & rtTexture : rtTextures)
//         {
//             renderpassProperties.renderpassAttachmentFormat.attachments.emplace_back(rtTexture->getFormat());
//         }
//     }
//
//     return renderpassProperties;
// }

GenericRenderPassProperties GlobalRenderingContextBase::renderpassPropsFromFb(const Framebuffer *fb) const
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

GenericRenderPassProperties
    GlobalRenderingContextBase::renderpassPropsFromRpFormat(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx) const
{
    // const Framebuffer* fb = GlobalBuffers::getFramebuffer(renderpassFormat, frameIdx);
    // if (fb)
    //{
    //     return renderpassPropsFromFb(fb);
    // }

    return GlobalBuffers::getFramebufferRenderpassProps(renderpassFormat);
}

const Framebuffer *GlobalRenderingContextBase::getFramebuffer(
    const GenericRenderPassProperties &renderpassProps, const std::vector<ImageResourceRef> &frameAttachments
) const
{
    auto renderpassFbs = rtFramebuffers.find(renderpassProps);

    if (renderpassFbs != rtFramebuffers.cend() && !renderpassFbs->second.empty())
    {
        if (renderpassProps.renderpassAttachmentFormat.attachments.empty())
        {
            // there can be only one render pass without any attachments.
            return renderpassFbs->second[0];
        }

        for (const Framebuffer *const &fb : renderpassFbs->second)
        {
            if (fb->textures.size() == frameAttachments.size())
            {
                bool bSameTextures = true;
                for (int32 attachmentIdx = 0; attachmentIdx < fb->textures.size(); ++attachmentIdx)
                {
                    bSameTextures = bSameTextures && fb->textures[attachmentIdx] == frameAttachments[attachmentIdx];
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

const Framebuffer *GlobalRenderingContextBase::createNewFramebuffer(
    const GenericRenderPassProperties &renderpassProps, const std::vector<ImageResourceRef> &frameAttachments
) const
{
    Framebuffer *fb = graphicsHelperCache->createFbInstance();
    fb->bHasResolves = !renderpassProps.bOneRtPerFormat;

    if (frameAttachments.empty())
    {
        graphicsHelperCache->initializeFb(graphicsInstanceCache, fb, Size2D());
    }
    else
    {
        fb->textures.insert(fb->textures.begin(), frameAttachments.cbegin(), frameAttachments.cend());
        graphicsHelperCache->initializeFb(
            graphicsInstanceCache, fb, Size2D(frameAttachments[0]->getImageSize().x, frameAttachments[0]->getImageSize().y)
        );
    }

    return fb;
}

const Framebuffer *GlobalRenderingContextBase::getOrCreateFramebuffer(
    const GenericRenderPassProperties &renderpassProps, const std::vector<ImageResourceRef> &frameAttachments
)
{
    const Framebuffer *fb = getFramebuffer(renderpassProps, frameAttachments);
    if (fb == nullptr)
    {
        fb = createNewFramebuffer(renderpassProps, frameAttachments);
        rtFramebuffers[renderpassProps].emplace_back(fb);
    }
    return fb;
}

PipelineBase *
    GlobalRenderingContextBase::createNewPipeline(UniqueUtilityShaderObject *shaderObject, const GenericRenderPassProperties &renderpassProps)
{
    fatalAssert(
        renderpassProps.renderpassAttachmentFormat.attachments.size()
            == shaderObject->getDefaultPipeline()->getRenderpassProperties().renderpassAttachmentFormat.attachments.size(),
        "Attachment count must be same for utility shader pipeline variants"
    );
    GraphicsPipelineBase *pipeline = static_cast<GraphicsPipelineBase *>(
        pipelineFactory->create(graphicsInstanceCache, graphicsHelperCache, { shaderObject->getShader(), shaderObject->getDefaultPipeline() })
    );
    pipeline->setRenderpassProperties(renderpassProps);

    initializeGenericGraphicsPipeline(shaderObject, pipeline);
    return pipeline;
}

void GlobalRenderingContextBase::preparePipelineContext(
    class LocalPipelineContext *pipelineContext, GenericRenderPassProperties renderpassProps
)
{
    std::unordered_map<String, ShaderDataCollection>::const_iterator shaderDataCollectionItr
        = rawShaderObjects.find(pipelineContext->materialName);
    if (shaderDataCollectionItr == rawShaderObjects.cend())
    {
        LOG_ERROR("GlobalRenderingContext", "%s : Requested material %s is not found", __func__, pipelineContext->materialName.getChar());
        return;
    }

    if (shaderDataCollectionItr->second.shaderObject->baseShaderType() == DrawMeshShaderConfig::staticType())
    {
        fatalAssert(!pipelineContext->frameAttachments.empty(), "%s() : Frame attachments cannot be empty");

        DrawMeshShaderObject *drawMeshShaderObj = static_cast<DrawMeshShaderObject *>(shaderDataCollectionItr->second.shaderObject);

        GraphicsPipelineBase *graphicsPipeline;
        drawMeshShaderObj->getShader(pipelineContext->forVertexType, FramebufferFormat(pipelineContext->renderpassFormat), &graphicsPipeline);
        pipelineContext->pipelineUsed = graphicsPipeline;

        // If empty RTs then get framebuffer from global buffers
        const Framebuffer *fb = nullptr;
        {
            renderpassProps.renderpassAttachmentFormat.rpFormat = pipelineContext->renderpassFormat;
            // To make sure that RTs created framebuffer is compatible with GlobalBuffers created FBs
            // and its render pass and pipelines
            fatalAssert(
                renderpassProps == renderpassPropsFromRpFormat(pipelineContext->renderpassFormat, pipelineContext->swapchainIdx),
                "%s() : Incompatible RTs for Mesh Draw shaders", __func__
            );

            fb = getOrCreateFramebuffer(renderpassProps, pipelineContext->frameAttachments);
        }
        fatalAssert(
            fb != nullptr, "%s() : Framebuffer is invalid[Shader : %s, Render pass format : %s]", __func__,
            pipelineContext->materialName.getChar(), ERenderPassFormat::toString(pipelineContext->renderpassFormat)
        );
        pipelineContext->framebuffer = fb;
    }
    else if (shaderDataCollectionItr->second.shaderObject->baseShaderType() == UniqueUtilityShaderConfig::staticType())
    {
        const Framebuffer *fb = nullptr;
        if (pipelineContext->windowCanvas.isValid())
        {
            auto itr = windowCanvasFramebuffers.find(pipelineContext->windowCanvas);
            if (itr == windowCanvasFramebuffers.end())
            {
                const uint32 imageCount = pipelineContext->windowCanvas->imagesCount();
                std::vector<const Framebuffer *> &fbs = windowCanvasFramebuffers[pipelineContext->windowCanvas];
                fbs.resize(imageCount);
                for (uint32 i = 0; i < imageCount; ++i)
                {
                    Framebuffer *newFb = graphicsHelperCache->createFbInstance();
                    graphicsHelperCache->initializeSwapchainFb(graphicsInstanceCache, newFb, pipelineContext->windowCanvas, i);
                    fbs[i] = newFb;
                }
                fb = fbs[pipelineContext->swapchainIdx];
            }
            else
            {
                fb = itr->second[pipelineContext->swapchainIdx];
            }

            renderpassProps = GenericRenderPassProperties();
            renderpassProps.bOneRtPerFormat = true;
            renderpassProps.multisampleCount = EPixelSampleCount::SampleCount1;
            renderpassProps.renderpassAttachmentFormat.attachments.resize(1);
            renderpassProps.renderpassAttachmentFormat.attachments[0] = pipelineContext->windowCanvas->windowCanvasFormat();
            renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;
        }
        else
        {
            fatalAssert(!pipelineContext->frameAttachments.empty(), "%s() : Frame attachments cannot be empty", __func__);
            fb = getOrCreateFramebuffer(renderpassProps, pipelineContext->frameAttachments);
        }
        UniqueUtilityShaderObject *uniqUtilShaderObj = static_cast<UniqueUtilityShaderObject *>(shaderDataCollectionItr->second.shaderObject);
        GraphicsPipelineBase *graphicsPipeline = uniqUtilShaderObj->getPipeline(renderpassProps);
        if (graphicsPipeline == nullptr)
        {
            graphicsPipeline = static_cast<GraphicsPipelineBase *>(createNewPipeline(uniqUtilShaderObj, renderpassProps));
            uniqUtilShaderObj->setPipeline(renderpassProps, graphicsPipeline);
        }
        pipelineContext->pipelineUsed = graphicsPipeline;
        pipelineContext->framebuffer = fb;
    }
    else if (shaderDataCollectionItr->second.shaderObject->baseShaderType() == ComputeShaderConfig::staticType())
    {
        ComputeShaderObject *computeShaderObj = static_cast<ComputeShaderObject *>(shaderDataCollectionItr->second.shaderObject);
        pipelineContext->pipelineUsed = computeShaderObj->getPipeline();
    }
}

void GlobalRenderingContextBase::clearExternInitRtsFramebuffer(
    const std::vector<ImageResourceRef> &frameAttachments, GenericRenderPassProperties renderpassProps
)
{
    auto renderpassFbs = rtFramebuffers.find(renderpassProps);
    if (renderpassFbs != rtFramebuffers.cend() && !renderpassFbs->second.empty())
    {
        if (renderpassProps.renderpassAttachmentFormat.attachments.empty())
        {
            // there can be only one render pass without any attachments.
            delete renderpassFbs->second[0];
            renderpassFbs->second.clear();
            return;
        }

        for (auto itr = renderpassFbs->second.begin(); itr != renderpassFbs->second.end(); ++itr)
        {
            const Framebuffer *fb = *itr;
            if (fb->textures.size() == frameAttachments.size())
            {
                bool bSameTextures = true;
                for (int32 attachmentIdx = 0; attachmentIdx < fb->textures.size(); ++attachmentIdx)
                {
                    bSameTextures = bSameTextures && fb->textures[attachmentIdx] == frameAttachments[attachmentIdx];
                }

                if (bSameTextures)
                {
                    delete fb;
                    renderpassFbs->second.erase(itr);
                    return;
                }
            }
        }
    }
}

void GlobalRenderingContextBase::clearWindowCanvasFramebuffer(WindowCanvasRef windowCanvas)
{
    auto itr = windowCanvasFramebuffers.find(windowCanvas);
    if (itr != windowCanvasFramebuffers.end())
    {
        for (const Framebuffer *fb : itr->second)
        {
            delete fb;
        }
    }
    windowCanvasFramebuffers.erase(itr);
}
