#include "RenderingContexts.h"
#include "../Shaders/Base/DrawMeshShader.h"
#include "../Shaders/Base/UtilityShaders.h"
#include "../ShaderCore/ShaderParameters.h"
#include "FramebufferTypes.h"
#include "../PlatformIndependentHeaders.h"
#include "../ShaderCore/ShaderParameterUtility.h"
#include "../../Core/Types/Textures/RenderTargetTextures.h"

#include <set>

void GlobalRenderingContextBase::initContext(IGraphicsInstance* graphicsInstance)
{
    initApiFactories();

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

            // TODO(Jeslas) : change below to init for all permutations under each material
        }
        else if (shader->getType()->isChildOf(UniqueUtilityShader::staticType()))
        {
            UniqueUtilityShader* utilityShader = static_cast<UniqueUtilityShader*>(shader);
            vertexAttribFillLambda(utilityShader->vertexUsage(), utilityShader->getReflection()->inputs);
        }
    }
}

void GlobalRenderingContextBase::clearContext()
{
    // TODO(Jeslas) : clear properly
    std::vector<GraphicsResource*> shaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(shaderResources, true);
    for (GraphicsResource* shader : shaderResources)
    {
        shader->release();
    }
}

GenericRenderpassProperties GlobalRenderingContextBase::renderpassPropsFromRts(const std::vector<RenderTargetTexture*>& rtTextures) const
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
