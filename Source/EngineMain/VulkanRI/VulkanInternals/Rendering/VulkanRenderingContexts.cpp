#include "VulkanRenderingContexts.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../../RenderInterface/ShaderCore/ShaderObjectFactory.h"
#include "../../../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "../../../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../../../RenderInterface/ShaderCore/ShaderObject.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"
#include "../../VulkanGraphicsHelper.h"
#include "../ShaderCore/VulkanShaderParamResourcesFactory.h"
#include "../ShaderCore/VulkanShaderParamResources.h"
#include "../Resources/VulkanPipelines.h"

void VulkanGlobalRenderingContext::initApiInstances()
{
    shaderParamLayoutsFactory = new VulkanShaderParametersLayoutFactory();
    pipelineFactory = new PipelineFactory();
    shaderObjectFactory = new ShaderObjectFactory();

    pipelinesCache = new VulkanPipelineCache();
}

void VulkanGlobalRenderingContext::initializeApiContext()
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();

    ShaderDataCollection& defaultShaderCollection = rawShaderObjects[DEFAULT_SHADER_NAME];
    {
        const std::vector<std::pair<const DrawMeshShader*, GraphicsPipelineBase*>>& defaultShaders
            = static_cast<DrawMeshShaderObject*>(defaultShaderCollection.shaderObject)->getAllShaders();
        for (const std::pair<const DrawMeshShader*, GraphicsPipelineBase*>& defaultShader : defaultShaders)
        {
            VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(defaultShader.second);
            VkRenderPass renderpass = createGbufferRenderpass(defaultShader.first->renderpassUsage(), {});

            // Since default alone will be used as parent
            graphicsPipeline->setCanBeParent(true);
            graphicsPipeline->setCompatibleRenderpass(renderpass);
            graphicsPipeline->pipelineLayout = VulkanGraphicsHelper::createPipelineLayout(graphicsInstance, graphicsPipeline);

            defaultShader.second->init();

            gbufferRenderPasses[defaultShader.first->renderpassUsage()].emplace_back(RenderpassPropsPair({}, renderpass));
            pipelineLayouts[defaultShader.first] = graphicsPipeline->pipelineLayout;
        }
    }

    for (std::pair<const String, ShaderDataCollection>& shaderCollection : rawShaderObjects)
    {
        if (shaderCollection.first == String(DEFAULT_SHADER_NAME))
        {
            continue;
        }

        if (shaderCollection.second.shaderObject->baseShaderType() == DrawMeshShader::staticType())
        {
            const std::vector<std::pair<const DrawMeshShader*, GraphicsPipelineBase*>>& allShaders
                = static_cast<DrawMeshShaderObject*>(shaderCollection.second.shaderObject)->getAllShaders();

            for (const std::pair<const DrawMeshShader*, GraphicsPipelineBase*>& shaderPair : allShaders)
            {
                FramebufferFormat fbFormat(shaderPair.first->renderpassUsage());
                GraphicsPipelineBase* defaultGraphicsPipeline;
                const DrawMeshShader* defaultShader = static_cast<DrawMeshShaderObject*>(defaultShaderCollection.shaderObject)
                    ->getShader(shaderPair.first->vertexUsage(), fbFormat, &defaultGraphicsPipeline);

                if (defaultShader == nullptr)
                {
                    Logger::error("VulkanGlobalRenderingContext", "%s : Default shader must contain all the permutations, Missing for [%s %s]"
                        , __func__, EVertexType::toString(shaderPair.first->vertexUsage()).getChar()
                        , ERenderPassFormat::toString(shaderPair.first->renderpassUsage()).getChar());
                    fatalAssert(defaultShader, "Default shader missing!");
                }

                shaderPair.second->setParentPipeline(defaultGraphicsPipeline);
                VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(shaderPair.second);
                graphicsPipeline->setCompatibleRenderpass(getRenderPass(shaderPair.first->renderpassUsage(), {}));
                graphicsPipeline->pipelineLayout = VulkanGraphicsHelper::createPipelineLayout(graphicsInstance, shaderPair.second);
                graphicsPipeline->init();

                pipelineLayouts[shaderPair.first] = graphicsPipeline->pipelineLayout;
            }
        }
        else if (shaderCollection.second.shaderObject->baseShaderType() == UniqueUtilityShader::staticType())
        {
            UniqueUtilityShaderObject* shaderObject = static_cast<UniqueUtilityShaderObject*>(shaderCollection.second.shaderObject);
            VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(shaderObject->getDefaultPipeline());
            graphicsPipeline->pipelineLayout = VulkanGraphicsHelper::createPipelineLayout(graphicsInstance, graphicsPipeline);

            initializeGenericGraphicsPipeline(shaderObject, shaderObject->getDefaultPipeline());
            pipelineLayouts[shaderObject->getShader()] = graphicsPipeline->pipelineLayout;
        }
    }
}

void VulkanGlobalRenderingContext::clearApiContext()
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    for (const std::pair<ShaderResource const* const, VkPipelineLayout>& pipelineLayout : pipelineLayouts)
    {
        VulkanGraphicsHelper::destroyPipelineLayout(graphicsInstance, pipelineLayout.second);
    }
    pipelineLayouts.clear();

    for (const std::pair<const ERenderPassFormat::Type, std::vector<RenderpassPropsPair>>& renderpasses : gbufferRenderPasses)
    {
        for (const RenderpassPropsPair& renderpass : renderpasses.second)
        {
            VulkanGraphicsHelper::destroyRenderPass(graphicsInstance, renderpass.second);
        }
    }
    gbufferRenderPasses.clear();

    for (const std::pair<const GenericRenderPassProperties, std::vector<RenderpassPropsPair>>& renderpasses : genericRenderPasses)
    {
        for (const RenderpassPropsPair& renderpass : renderpasses.second)
        {
            VulkanGraphicsHelper::destroyRenderPass(graphicsInstance, renderpass.second);
        }
    }
    genericRenderPasses.clear();
}

VkRenderPass VulkanGlobalRenderingContext::createGbufferRenderpass(ERenderPassFormat::Type rpUsageFormat
    , const RenderPassAdditionalProps& additionalProps) const
{
    GenericRenderPassProperties renderpassProps;
    renderpassProps.multisampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    renderpassProps.renderpassAttachmentFormat.rpFormat = rpUsageFormat;
    Framebuffer* fb = GBuffers::getFramebuffer(renderpassProps.renderpassAttachmentFormat, 0);
    renderpassProps.bOneRtPerFormat = !fb->bHasResolves;

    return VulkanGraphicsHelper::createRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), renderpassProps, additionalProps);
}

void VulkanGlobalRenderingContext::initializeGenericGraphicsPipeline(UniqueUtilityShaderObject* shaderObject, PipelineBase* pipeline)
{
    VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(pipeline);
    GenericRenderPassProperties renderPassProps = graphicsPipeline->getRenderpassProperties();

    VkRenderPass renderPass = nullptr;
    auto renderpassItr = genericRenderPasses.find(renderPassProps);
    if (renderpassItr == genericRenderPasses.end())
    {
        renderPass = VulkanGraphicsHelper::createRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), renderPassProps, {});
        genericRenderPasses[renderPassProps].emplace_back(RenderpassPropsPair({}, renderPass));
    }
    else
    {
        renderPass = renderpassItr->second[0].second;
    }

    graphicsPipeline->setCompatibleRenderpass(renderPass);
    graphicsPipeline->init();
}

VkRenderPass VulkanGlobalRenderingContext::getRenderPass(ERenderPassFormat::Type renderpassFormat, const RenderPassAdditionalProps& additionalProps)
{
    auto renderpassItr = gbufferRenderPasses.find(renderpassFormat);
    if (renderpassItr != gbufferRenderPasses.end())
    {
        VkRenderPass renderpass = nullptr;
        for (const RenderpassPropsPair& renderpassPair : renderpassItr->second)
        {
            if (renderpassPair.first == additionalProps)
            {
                renderpass = renderpassPair.second;
                break;
            }
        }

        if (renderpass == nullptr)
        {
            renderpass = createGbufferRenderpass(renderpassFormat, additionalProps);
            renderpassItr->second.emplace_back(RenderpassPropsPair(additionalProps, renderpass));
        }
        return renderpass;
    }
    return nullptr;
}

VkRenderPass VulkanGlobalRenderingContext::getRenderPass(const GenericRenderPassProperties& renderpassProps, const RenderPassAdditionalProps& additionalProps)
{
    auto renderpassItr = genericRenderPasses.find(renderpassProps);
    if (renderpassItr != genericRenderPasses.end())
    {
        VkRenderPass renderpass = nullptr;
        for (const RenderpassPropsPair& renderpassPair : renderpassItr->second)
        {
            if (renderpassPair.first == additionalProps)
            {
                renderpass = renderpassPair.second;
                break;
            }
        }

        if (renderpass == nullptr)
        {
            renderpass = VulkanGraphicsHelper::createRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), renderpassProps, additionalProps);
            renderpassItr->second.emplace_back(RenderpassPropsPair(additionalProps, renderpass));
        }
        return renderpass;
    }
    return nullptr;
}

VkPipelineLayout VulkanGraphicsHelper::createPipelineLayout(class IGraphicsInstance* graphicsInstance, const class PipelineBase* pipeline)
{
    std::vector<VkDescriptorSetLayout> descSetLayouts;
    const ShaderResource* shaderResource = pipeline->getShaderResource();


    if (shaderResource->getType()->isChildOf(DrawMeshShader::staticType()))
    {
        for (const ReflectDescriptorBody& reflectDescBody : shaderResource->getReflection()->descriptorsSets)
        {
            if (descSetLayouts.size() <= reflectDescBody.set)
            {
                descSetLayouts.resize(reflectDescBody.set + 1);
            }
            const VulkanShaderSetParamsLayout* shaderSetParamsLayout = static_cast<const VulkanShaderSetParamsLayout*>(
                pipeline->getParamLayoutAtSet(reflectDescBody.set));
            descSetLayouts[reflectDescBody.set] = shaderSetParamsLayout->descriptorLayout;
        }
    }
    else if (shaderResource->getType()->isChildOf(UniqueUtilityShader::staticType()))
    {
        const VulkanShaderParametersLayout* shaderParametersLayout = static_cast<const VulkanShaderParametersLayout*>(
            pipeline->getParamLayoutAtSet(0));

        for (const ReflectDescriptorBody& reflectDescBody : shaderResource->getReflection()->descriptorsSets)
        {
            if (descSetLayouts.size() <= reflectDescBody.set)
            {
                descSetLayouts.resize(reflectDescBody.set + 1);
            }
            descSetLayouts[reflectDescBody.set] = shaderParametersLayout->getDescSetLayout(reflectDescBody.set);
        }
    }

    PIPELINE_LAYOUT_CREATE_INFO(layoutCreateInfo);

    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = shaderResource->getReflection()->pushConstants.data.pushConstantField.stride;
    pushConstant.stageFlags = shaderResource->getReflection()->pushConstants.data.stagesUsed;

    if (shaderResource->getReflection()->pushConstants.data.stagesUsed > 0 && shaderResource->getReflection()->pushConstants.data.pushConstantField.stride > 0)
    {
        layoutCreateInfo.pushConstantRangeCount = 1;
        layoutCreateInfo.pPushConstantRanges = &pushConstant;
    }
    else
    {
        layoutCreateInfo.pushConstantRangeCount = 0;
        layoutCreateInfo.pPushConstantRanges = nullptr;
    }
    layoutCreateInfo.setLayoutCount = uint32(descSetLayouts.size());
    layoutCreateInfo.pSetLayouts = descSetLayouts.data();

    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    VkPipelineLayout pipelineLayout;
    if (device->vkCreatePipelineLayout(device->logicalDevice, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        Logger::error("VulkanGraphicsHelper", "%s : Pipeline layout creation failed for shader %s", __func__, shaderResource->getResourceName().getChar());
        pipelineLayout = nullptr;
    }

    return pipelineLayout;
}
