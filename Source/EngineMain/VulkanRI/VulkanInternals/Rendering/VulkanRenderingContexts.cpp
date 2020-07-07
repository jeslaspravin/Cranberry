#include "VulkanRenderingContexts.h"
#include "../ShaderCore/VulkanShaderParamResourcesFactory.h"
#include "../../../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "../../../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../../../RenderInterface/ShaderCore/ShaderObject.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"
#include "../../VulkanGraphicsHelper.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../ShaderCore/VulkanShaderParamResources.h"
#include "../../../Core/Logger/Logger.h"
#include "../Resources/VulkanPipelines.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"

void VulkanGlobalRenderingContext::initApiFactories()
{
    shaderParamLayoutsFactory = new VulkanShaderParametersLayoutFactory();
    pipelineFactory = new PipelineFactory();
}

void VulkanGlobalRenderingContext::initializeApiContext()
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();

    ShaderDataCollection& defaultShaderCollection = rawShaderObjects[DEFAULT_SHADER_NAME];
    {
        const std::vector<std::pair<const DrawMeshShader*, PGraphicsPipeline>>& defaultShaders
            = static_cast<DrawMeshShaderObject*>(defaultShaderCollection.shaderObject)->getAllShaders();
        for (const std::pair<const DrawMeshShader*, PGraphicsPipeline>& defaultShader : defaultShaders)
        {
            VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(defaultShader.second);
            graphicsPipeline->compatibleRenderpass = createGbufferRenderpass(defaultShader.first);
            graphicsPipeline->pipelineLayout = VulkanGraphicsHelper::createPipelineLayout(graphicsInstance, graphicsPipeline);

            defaultShader.second->init();

            gbufferRenderpasses[defaultShader.first->renderpassUsage()] = graphicsPipeline->compatibleRenderpass;
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
            const std::vector<std::pair<const DrawMeshShader*, PGraphicsPipeline>>& allShaders
                = static_cast<DrawMeshShaderObject*>(shaderCollection.second.shaderObject)->getAllShaders();

            for (const std::pair<const DrawMeshShader*, PGraphicsPipeline>& shaderPair : allShaders)
            {
                FramebufferFormat fbFormat(shaderPair.first->renderpassUsage());
                PGraphicsPipeline defaultGraphicsPipeline;
                const DrawMeshShader* defaultShader = static_cast<DrawMeshShaderObject*>(defaultShaderCollection.shaderObject)
                    ->getShader(shaderPair.first->vertexUsage(), fbFormat, &defaultGraphicsPipeline);

                if (defaultShader == nullptr)
                {
                    Logger::error("VulkanGlobalRenderingContext", "%s : Default shader must contain all the permutations, Missing for [%s %s]"
                        , __func__, EVertexType::toString(shaderPair.first->vertexUsage()).getChar()
                        , ERenderpassFormat::toString(shaderPair.first->renderpassUsage()).getChar());
                    fatalAssert(defaultShader, "Default shader missing!");
                }

                shaderPair.second->setParentPipeline(defaultGraphicsPipeline);
                VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(shaderPair.second);
                graphicsPipeline->compatibleRenderpass = static_cast<VulkanGraphicsPipeline*>(defaultGraphicsPipeline)->compatibleRenderpass;
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

            initializeNewPipeline(shaderObject, shaderObject->getDefaultPipeline());
        }
    }
}

void VulkanGlobalRenderingContext::clearApiContext()
{
    // TODO(Jeslas) : Asap
}

VkRenderPass VulkanGlobalRenderingContext::createGbufferRenderpass(const DrawMeshShader* shaderResource) const
{
    GenericRenderpassProperties renderpassProps;
    renderpassProps.multisampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    renderpassProps.renderpassAttachmentFormat.rpFormat = shaderResource->renderpassUsage();
    Framebuffer* fb = GBuffers::getFramebuffer(renderpassProps.renderpassAttachmentFormat, 0);
    renderpassProps.bOneRtPerFormat = !fb->bHasResolves;

    return VulkanGraphicsHelper::createRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), renderpassProps);
}

void VulkanGlobalRenderingContext::initializeNewPipeline(UniqueUtilityShaderObject* shaderObject, PipelineBase* pipeline)
{
    VulkanGraphicsPipeline* graphicsPipeline = static_cast<VulkanGraphicsPipeline*>(shaderObject->getDefaultPipeline());
    GenericRenderpassProperties renderpassProps = graphicsPipeline->getRenderpassProperties();

    VkRenderPass renderpass;
    auto renderpassItr = genericRenderpasses.find(renderpassProps);
    if (renderpassItr == genericRenderpasses.end())
    {
        renderpass = VulkanGraphicsHelper::createRenderPass(gEngine->getRenderApi()->getGraphicsInstance(), renderpassProps);
        genericRenderpasses[renderpassProps] = renderpass;
    }

    graphicsPipeline->compatibleRenderpass = renderpass;
    graphicsPipeline->init();
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
                static_cast<const GraphicsPipeline*>(pipeline)->getParamLayoutAtSet(reflectDescBody.set));
            descSetLayouts[reflectDescBody.set] = shaderSetParamsLayout->descriptorLayout;
        }
    }
    else if (shaderResource->getType()->isChildOf(UniqueUtilityShader::staticType()))
    {
        const VulkanShaderParametersLayout* shaderParametersLayout = static_cast<const VulkanShaderParametersLayout*>(
            static_cast<const GraphicsPipeline*>(pipeline)->getParamLayoutAtSet(0));

        for (const ReflectDescriptorBody& reflectDescBody : shaderResource->getReflection()->descriptorsSets)
        {
            if (descSetLayouts.size() <= reflectDescBody.set)
            {
                descSetLayouts.resize(reflectDescBody.set + 1);
            }
            descSetLayouts[reflectDescBody.set] = shaderParametersLayout->getDescSetLayout(reflectDescBody.set);
        }
    }

    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = shaderResource->getReflection()->pushConstants.data.pushConstantField.stride;
    pushConstant.stageFlags = shaderResource->getReflection()->pushConstants.data.stagesUsed;

    PIPELINE_LAYOUT_CREATE_INFO(layoutCreateInfo);
    layoutCreateInfo.pushConstantRangeCount = 1;
    layoutCreateInfo.pPushConstantRanges = &pushConstant;
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
