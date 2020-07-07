#include "RenderingContexts.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../PlatformIndependentHeaders.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../ShaderCore/ShaderParameters.h"
#include "FramebufferTypes.h"
#include "../ShaderCore/ShaderParameterUtility.h"
#include "../../Core/Types/Textures/RenderTargetTextures.h"
#include "../ShaderCore/ShaderObject.h"
#include "../Resources/Pipelines.h"
#include "../../RenderApi/GBuffersAndTextures.h"

void GlobalRenderingContextBase::initContext(IGraphicsInstance* graphicsInstance)
{
    initApiFactories();

    initShaderResources();

    initializeApiContext();
}

void GlobalRenderingContextBase::clearContext()
{
    delete pipelineFactory;
    delete shaderObjectFactory;
    delete shaderParamLayoutsFactory;

    clearApiContext();
    destroyShaderResources();
}

void GlobalRenderingContextBase::initShaderResources()
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

    std::vector<GraphicsResource*> defaultModeShaders;
    GraphicsShaderResource::staticType()->allChildDefaultResources(defaultModeShaders);// TODO(Jeslas) : change this to only leaf childs
    for (GraphicsResource* shader : defaultModeShaders)
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
            PGraphicsPipeline graphicsPipeline = static_cast<GraphicsPipeline*>(pipelineFactory->create({ drawMeshShader }));
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout, 2);
            graphicsPipeline->setParamLayoutAtSet(perVertexTypeLayouts[drawMeshShader->vertexUsage()], 1);
            graphicsPipeline->setParamLayoutAtSet(sceneViewParamLayout, 0);
            graphicsPipeline->setPipelineShader(drawMeshShader);
            GenericRenderpassProperties renderpassProp;
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

            PGraphicsPipeline graphicsPipeline = static_cast<PGraphicsPipeline>(pipelineFactory->create({ utilityShader }));
            graphicsPipeline->setParamLayoutAtSet(shaderCollection.shadersParamLayout);
            graphicsPipeline->setPipelineShader(utilityShader);

            static_cast<UniqueUtilityShaderObject*>(shaderCollection.shaderObject)->setPipeline(graphicsPipeline->getRenderpassProperties()
                , graphicsPipeline);
        }
    }
}

void GlobalRenderingContextBase::destroyShaderResources()
{
    // TODO(Jeslas) : clear properly
    std::vector<GraphicsResource*> shaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(shaderResources, true);
    for (GraphicsResource* shader : shaderResources)
    {
        shader->release();
    }
}

GenericRenderpassProperties GlobalRenderingContextBase::renderpassPropsFromRTs(const std::vector<RenderTargetTexture*>& rtTextures) const
{
    GenericRenderpassProperties renderpassProperties;
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

const Framebuffer* GlobalRenderingContextBase::getFramebuffer(const GenericRenderpassProperties& renderpassProps, const std::vector<RenderTargetTexture*>& rtTextures) const
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

const Framebuffer* GlobalRenderingContextBase::createNewFramebuffer(const GenericRenderpassProperties& renderpassProps
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
    , const GenericRenderpassProperties& renderpassProps)
{
    GraphicsPipeline* pipeline = static_cast<GraphicsPipeline*>(pipelineFactory->create({ shaderObject->getShader(), shaderObject->getDefaultPipeline() }));
    pipeline->setRenderpassProperties(renderpassProps);

    initializeNewPipeline(shaderObject, pipeline);
    return pipeline;
}
