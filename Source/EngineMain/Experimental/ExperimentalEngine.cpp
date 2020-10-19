#include "ExperimentalEngine.h"

#include "../RenderInterface/Shaders/EngineShaders/GoochModelShader.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanQueueResource.h"
#include "../VulkanRI/VulkanInternals/Debugging.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../RenderInterface/PlatformIndependentHelper.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../VulkanRI/VulkanInternals/VulkanDescriptorAllocator.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSampler.h"
#include "../Core/Math/Vector3D.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../Assets/Asset/TextureAsset.h"
#include "../Core/Types/Textures/TexturesBase.h"
#include "../RenderApi/GBuffersAndTextures.h"
#include "../Core/Input/Keys.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../VulkanRI/VulkanGraphicsHelper.h"
#include "../RenderApi/RenderApi.h"
#include "../Core/Platform/GenericAppInstance.h"
#include "../Core/Input/InputSystem.h"
#include "../Core/Math/Math.h"
#include "../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "../RenderInterface/CoreGraphicsTypes.h"
#include "../VulkanRI/VulkanInternals/ShaderCore/VulkanShaderParamResources.h"
#include "../RenderApi/Scene/RenderScene.h"
#include "../RenderApi/Material/MaterialCommonUniforms.h"
#include "../Core/Math/RotationMatrix.h"
#include "../Core/Types/Textures/RenderTargetTextures.h"
#include "../RenderInterface/GlobalRenderVariables.h"

#include <array>
#include <random>

void ExperimentalEngine::tempTest()
{

}

void ExperimentalEngine::tempTestPerFrame()
{

}

template <EQueueFunction QueueFunction> VulkanQueueResource<QueueFunction>* getQueue(const VulkanDevice* device);

void ExperimentalEngine::createPools()
{
    {
        VulkanQueueResource<EQueueFunction::Compute>* queue = getQueue<EQueueFunction::Compute>(vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Compute];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Compute_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Compute_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Compute_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
    {
        VulkanQueueResource<EQueueFunction::Graphics>* queue = getQueue<EQueueFunction::Graphics>(vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Graphics];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Graphics_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Graphics_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Graphics_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
    {
        VulkanQueueResource<EQueueFunction::Transfer>* queue = getQueue<EQueueFunction::Transfer>(vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Transfer];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Transfer_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Transfer_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Transfer_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
    {
        VulkanQueueResource<EQueueFunction::Present>* queue = getQueue<EQueueFunction::Present>(vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Present];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Present_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Present_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Present_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
}

void ExperimentalEngine::destroyPools()
{
    for (const std::pair<const EQueueFunction, QueueCommandPool>& pool : pools)
    {
        vDevice->vkDestroyCommandPool(device, pool.second.oneTimeRecordPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.resetableCommandPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.tempCommandsPool, nullptr);
    }
}

void ExperimentalEngine::createImages()
{
    nearestFiltering = GraphicsHelper::createSampler(gEngine->getRenderApi()->getGraphicsInstance(), "NearestSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Nearest);
    linearFiltering = GraphicsHelper::createSampler(gEngine->getRenderApi()->getGraphicsInstance(), "LinearSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Linear);

    // common shader sampling texture
    {
        texture.image = static_cast<TextureAsset*>(appInstance().assetManager.getOrLoadAsset("TestImageData.png"))->getTexture();
        texture.imageView = static_cast<VulkanImageResource*>(texture.image->getTextureResource())->getImageView({});

        if (texture.imageView != nullptr)
        {
            graphicsDbg->markObject((uint64)texture.imageView, "DiffuseTextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void ExperimentalEngine::destroyImages()
{
    nearestFiltering->release();
    linearFiltering->release();
}

void ExperimentalEngine::createScene()
{
    StaticMeshAsset* cube = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cube.obj"));
    StaticMeshAsset* sphere = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Sphere.obj"));
    StaticMeshAsset* cylinder = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cylinder.obj"));
    StaticMeshAsset* cone = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cone.obj"));

    SceneEntity sceneFloor;
    sceneFloor.meshAsset = cube;
    sceneFloor.transform.setScale(Vector3D(10, 10, 1));
    sceneFloor.transform.setTranslation(Vector3D(0, 0, -50));
    sceneFloor.meshBatchColors.emplace_back(LinearColor(0.80f, 0.78f, 0.60f));

    sceneData.emplace_back(sceneFloor);

    // Ceiling
    sceneFloor.transform.setTranslation(Vector3D(0, 0, 550));
    sceneData.emplace_back(sceneFloor);

    // Pillars
    sceneFloor.meshAsset = cylinder;
    sceneFloor.transform.setScale(Vector3D(1, 1, 5));
    sceneFloor.transform.setTranslation(Vector3D(450, 450, 250));
    sceneData.emplace_back(sceneFloor);
    sceneFloor.transform.setTranslation(Vector3D(-450, 450, 250));
    sceneData.emplace_back(sceneFloor);
    sceneFloor.transform.setTranslation(Vector3D(450, -450, 250));
    sceneData.emplace_back(sceneFloor);
    sceneFloor.transform.setTranslation(Vector3D(-450, -450, 250));
    sceneData.emplace_back(sceneFloor);

    std::array<StaticMeshAsset*, 4> assets{ cube, sphere, cylinder, cone };

    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    std::normal_distribution<float> distribution1(0.0, 1.0);
    for (uint32 i = 0; i < 5; ++i)
    {
        SceneEntity entity;
        entity.meshAsset = assets[std::rand() % assets.size()];
        entity.transform.setTranslation(Vector3D(distribution(generator) * 400, distribution(generator) * 400, distribution1(generator) * 100 + 50));
        entity.transform.setRotation(Rotation(0, 0, distribution(generator) * 45));

        entity.meshBatchColors.emplace_back(LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1));
        sceneData.emplace_back(entity);
    }

    GoochModelLightData light;
    light.warmOffsetAndPosX = Vector4D(0.3f, 0.3f, 0.0f, 0);
    light.coolOffsetAndPosY = Vector4D(0.0f, 0.0f, 0.55f, 0);

    // Near floor
    float height = 150;

    // Middle light
    light.highlightColorAndPosZ = Vector4D(1.f, 1.f, 1.f, height);
    light.lightColorAndRadius = Vector4D(1.f, 1.f, 1.f, 0);
    lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });

    // Light 1
    light.highlightColorAndPosZ = Vector4D(0.49f, 0.66f, 0.75f, height);
    light.lightColorAndRadius = Vector4D(0.45f, 0.58f, 0.80f, 0);

    light.warmOffsetAndPosX.w() = light.coolOffsetAndPosY.w() = 400;
    lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
    // Light 2
    light.coolOffsetAndPosY.w() = -light.coolOffsetAndPosY.w();
    lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
    // Light 3
    light.warmOffsetAndPosX.w() = -light.warmOffsetAndPosX.w();
    lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
    // Light 4
    light.coolOffsetAndPosY.w() = -light.coolOffsetAndPosY.w();
    lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });

    //// Near ceiling
    //height = 400;
    //// Light 5
    //light.warmOffsetAndPosX.w() = light.coolOffsetAndPosY.w() = 400;
    //lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
    //// Light 6
    //light.coolOffsetAndPosY.w() = -light.coolOffsetAndPosY.w();
    //lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
    //// Light 7
    //light.warmOffsetAndPosX.w() = -light.warmOffsetAndPosX.w();
    //lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
    //// Light 8
    //light.coolOffsetAndPosY.w() = -light.coolOffsetAndPosY.w();
    //lightData.emplace_back(std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>{ light, {} });
}

void ExperimentalEngine::destroyScene()
{
    sceneData.clear();
}

void ExperimentalEngine::createShaderParameters()
{
    IGraphicsInstance* graphicsInstance = getRenderApi()->getGraphicsInstance();
    const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmPipelineContext.getPipeline());
    // Since view data and other view related data are at set 0
    viewParameters = GraphicsHelper::createShaderParameters(graphicsInstance, smPipeline->getParamLayoutAtSet(0));
    viewParameters->setResourceName("View");
    for (SceneEntity& entity : sceneData)
    {
        entity.instanceParameters = GraphicsHelper::createShaderParameters(graphicsInstance, smPipeline->getParamLayoutAtSet(1));
        entity.instanceParameters->setResourceName(entity.meshAsset->assetName());
        entity.meshBatchParameters.resize(entity.meshAsset->meshBatches.size());
        uint32 meshBatchIdx = 0;
        for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
        {
            meshBatchParam = (GraphicsHelper::createShaderParameters(graphicsInstance, smPipeline->getParamLayoutAtSet(2)));
            meshBatchParam->setResourceName(entity.meshAsset->assetName() + "_MeshBatch_" + std::to_string(meshBatchIdx));
            ++meshBatchIdx;
        }
    }

    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    lightTextures.resize(swapchainCount);
    drawQuadTextureDescs.resize(swapchainCount);
    drawQuadNormalDescs.resize(swapchainCount);
    drawQuadDepthDescs.resize(swapchainCount);
    drawLitColorsDescs.resize(swapchainCount);

    // Light related descriptors
    // as 1 and 2 are textures and light data
    const GraphicsResource* goochModelDescLayout = drawGoochPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    lightCommon = GraphicsHelper::createShaderParameters(graphicsInstance, goochModelDescLayout, { 1,2 });
    lightCommon->setResourceName("LightCommon");
    uint32 lightIdx = 0;
    for (std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>& light : lightData)
    {
        // as 0 and 1 are light common and textures
        light.second = GraphicsHelper::createShaderParameters(graphicsInstance, goochModelDescLayout, { 0, 1 });
        light.second->setResourceName("Light" + std::to_string(lightIdx));
        ++lightIdx;
    }

    const GraphicsResource* drawQuadDescLayout = drawQuadPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        const String iString = std::to_string(i);
        lightTextures[i] = GraphicsHelper::createShaderParameters(graphicsInstance, goochModelDescLayout, { 0,2 });
        lightTextures[i]->setResourceName("LightFrameCommon_" + iString);
        drawQuadTextureDescs[i] = GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout);
        drawQuadTextureDescs[i]->setResourceName("QuadUnlit_" + iString);
        drawQuadNormalDescs[i] = GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout);
        drawQuadNormalDescs[i]->setResourceName("QuadNormal_" + iString);
        drawQuadDepthDescs[i] = GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout);
        drawQuadDepthDescs[i]->setResourceName("QuadDepth_" + iString);
        drawLitColorsDescs[i] = GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout);
        drawLitColorsDescs[i]->setResourceName("QuadLit_" + iString);
    }

    clearInfoParams = GraphicsHelper::createShaderParameters(graphicsInstance, clearQuadPipelineContext.getPipeline()->getParamLayoutAtSet(0));
    clearInfoParams->setResourceName("ClearInfo");
}

void ExperimentalEngine::setupShaderParameterParams()
{
    IGraphicsInstance* graphicsInstance = getRenderApi()->getGraphicsInstance();

    ViewData viewData;
    viewData.view = camera.viewMatrix();
    viewData.invView = viewData.view.inverse();
    viewData.projection = camera.projectionMatrix();
    viewData.invProjection = viewData.projection.inverse();
    viewParameters->setBuffer("viewData", viewData);
    viewParameters->init();
    for (SceneEntity& entity : sceneData)
    {
        entity.instanceParameters->setMatrixParam("model", entity.transform.getTransformMatrix());
        entity.instanceParameters->setMatrixParam("invModel", entity.transform.getTransformMatrix().inverse());
        entity.instanceParameters->init();

        uint32 batchIdx = 0;
        for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
        {
            meshBatchParam->setVector4Param("meshColor", Vector4D(entity.meshBatchColors[batchIdx].getColorValue()));
            meshBatchParam->init();
            ++batchIdx;
        }
    }

    lightCommon->setBuffer("viewData", viewData);
    lightCommon->setIntParam("lightsCount", uint32(lightData.size()));
    lightCommon->setFloatParam("invLightsCount", 1.0f / lightData.size());
    lightCommon->init();
    for (std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>& light : lightData)
    {
        light.second->setBuffer("light", light.first);
        light.second->init();
    }

    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* multibuffer = GBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, i);
        lightTextures[i]->setTextureParam("ssUnlitColor", multibuffer->textures[1], nearestFiltering);
        lightTextures[i]->setTextureParam("ssNormal", multibuffer->textures[3], nearestFiltering);
        lightTextures[i]->setTextureParam("ssDepth", multibuffer->textures[5], nearestFiltering);
        lightTextures[i]->setTextureParam("ssColor", frameResources[i].lightingPassRt->getTextureResource(), nearestFiltering);
        lightTextures[i]->init();

        drawQuadTextureDescs[i]->setTextureParam("quadTexture", multibuffer->textures[1], linearFiltering);
        drawQuadTextureDescs[i]->init();
        drawQuadNormalDescs[i]->setTextureParam("quadTexture", multibuffer->textures[3], linearFiltering);
        drawQuadNormalDescs[i]->init();
        drawQuadDepthDescs[i]->setTextureParam("quadTexture", multibuffer->textures[5], linearFiltering);
        drawQuadDepthDescs[i]->init();
        drawLitColorsDescs[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
        drawLitColorsDescs[i]->init();
    }

    clearInfoParams->setVector4Param("clearColor", Vector4D(0, 0, 0, 0));
    clearInfoParams->init();
}

void ExperimentalEngine::updateShaderParameters(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    //const bool canUpdate = timeData.frameCounter % frameResources.size() == 0;

    // Update once every swapchain cycles are presented
    //if(canUpdate)
    {
        //for (const FrameResource& frameRes : frameResources)
        //{
        //    if (!frameRes.recordingFence->isSignaled())
        //    {
        //        frameRes.recordingFence->waitForSignal();
        //    }
        //}
        viewParameters->updateParams(cmdList, graphicsInstance);
        for (SceneEntity& entity : sceneData)
        {
            entity.instanceParameters->updateParams(cmdList, graphicsInstance);

            for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
            {
                meshBatchParam->updateParams(cmdList, graphicsInstance);
            }
        }

        lightCommon->updateParams(cmdList, graphicsInstance);
        for (std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>& light : lightData)
        {
            light.second->updateParams(cmdList, graphicsInstance);
        }

        uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
        for (uint32 i = 0; i < swapchainCount; ++i)
        {
            lightTextures[i]->updateParams(cmdList, graphicsInstance);
            drawQuadTextureDescs[i]->updateParams(cmdList, graphicsInstance);
            drawQuadNormalDescs[i]->updateParams(cmdList, graphicsInstance);
            drawQuadDepthDescs[i]->updateParams(cmdList, graphicsInstance);
            drawLitColorsDescs[i]->updateParams(cmdList, graphicsInstance);
        }

        clearInfoParams->updateParams(cmdList, graphicsInstance);
    }
}

void ExperimentalEngine::reupdateTextureParamsOnResize()
{
    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* multibuffer = GBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, i);
        lightTextures[i]->setTextureParam("ssUnlitColor", multibuffer->textures[1], nearestFiltering);
        lightTextures[i]->setTextureParam("ssNormal", multibuffer->textures[3], nearestFiltering);
        lightTextures[i]->setTextureParam("ssDepth", multibuffer->textures[5], nearestFiltering);
        lightTextures[i]->setTextureParam("ssColor", frameResources[i].lightingPassRt->getTextureResource(), nearestFiltering);

        drawQuadTextureDescs[i]->setTextureParam("quadTexture", multibuffer->textures[1], linearFiltering);
        drawQuadNormalDescs[i]->setTextureParam("quadTexture", multibuffer->textures[3], linearFiltering);
        drawQuadDepthDescs[i]->setTextureParam("quadTexture", multibuffer->textures[5], linearFiltering);
        drawLitColorsDescs[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
    }
}

void ExperimentalEngine::destroyShaderParameters()
{
    viewParameters->release();
    viewParameters.reset();

    for (SceneEntity& entity : sceneData)
    {
        entity.instanceParameters->release();
        entity.instanceParameters.reset();
        for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
        {
            meshBatchParam->release();
        }
        entity.meshBatchParameters.clear();
    }

    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    
    lightCommon->release();
    lightCommon.reset();

    for (std::pair<GoochModelLightData, SharedPtr<ShaderParameters>>& light : lightData)
    {
        light.second->release();
        light.second.reset();
    }

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        lightTextures[i]->release();
        drawQuadTextureDescs[i]->release();
        drawQuadNormalDescs[i]->release();
        drawQuadDepthDescs[i]->release();
        drawLitColorsDescs[i]->release();
    }
    lightTextures.clear();
    drawQuadTextureDescs.clear();
    drawQuadNormalDescs.clear();
    drawQuadDepthDescs.clear();
    drawLitColorsDescs.clear();

    clearInfoParams->release();
    clearInfoParams.reset();
}

void ExperimentalEngine::resizeLightingRts(const Size2D& size)
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        frameResources[i].lightingPassRt->setTextureSize(size);
        getRenderApi()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
    }
}

void ExperimentalEngine::createFrameResources()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    std::vector<VkCommandBuffer> cmdBuffers(windowCanvas->imagesCount());

    CMD_BUFFER_ALLOC_INFO(cmdBufAllocInfo);
    cmdBufAllocInfo.commandPool = pools[EQueueFunction::Graphics].resetableCommandPool;
    cmdBufAllocInfo.commandBufferCount = windowCanvas->imagesCount();
    vDevice->vkAllocateCommandBuffers(device, &cmdBufAllocInfo, cmdBuffers.data());

    RenderTextureCreateParams rtCreateParams;
    rtCreateParams.bSameReadWriteTexture = false;
    rtCreateParams.filtering = ESamplerFiltering::Linear;
    rtCreateParams.format = ERenderTargetFormat::RT_U8;
    rtCreateParams.sampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    rtCreateParams.textureSize = EngineSettings::screenSize.get();

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        String name = "Frame";
        name.append(std::to_string(i));

        frameResources[i].perFrameCommands = cmdBuffers[i];
        frameResources[i].usageWaitSemaphore.push_back(GraphicsHelper::createSemaphore(getRenderApi()->getGraphicsInstance(), (name + "QueueSubmit").c_str()));
        frameResources[i].recordingFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), (name + "RecordingGaurd").c_str(),true);

        rtCreateParams.textureName = "LightingRT_" + std::to_string(i);
        frameResources[i].lightingPassRt = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
    }
}

void ExperimentalEngine::destroyFrameResources()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    std::vector<VkCommandBuffer> cmdBuffers(windowCanvas->imagesCount());
    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        cmdBuffers[i] = frameResources[i].perFrameCommands;
        frameResources[i].usageWaitSemaphore[0]->release();
        frameResources[i].recordingFence->release();
        frameResources[i].perFrameCommands = nullptr;
        frameResources[i].usageWaitSemaphore[0].reset();
        frameResources[i].recordingFence.reset();

        getRenderApi()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassRt);
    }

    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Graphics].resetableCommandPool, (uint32)cmdBuffers.size(), cmdBuffers.data());
}

void ExperimentalEngine::getPipelineForSubpass()
{
    VulkanGlobalRenderingContext* vulkanRenderingContext = static_cast<VulkanGlobalRenderingContext*>(getRenderApi()->getGlobalRenderingContext());

    drawSmPipelineContext.forVertexType = EVertexType::StaticMesh;
    drawSmPipelineContext.materialName = "SingleColor";
    drawSmPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffers;
    drawSmPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&drawSmPipelineContext);
    drawSmRenderPass = vulkanRenderingContext->getRenderPass(drawSmPipelineContext.renderpassFormat, {});

    // Gooch model
    drawGoochPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawGoochPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassRt);
    drawGoochPipelineContext.swapchainIdx = 0;
    drawGoochPipelineContext.materialName = "GoochModel";
    vulkanRenderingContext->preparePipelineContext(&drawGoochPipelineContext);
    RenderPassAdditionalProps additionalProps;
    additionalProps.bAllowUndefinedLayout = false;
    lightingRenderPass = vulkanRenderingContext->getRenderPass(
        static_cast<const GraphicsPipelineBase*>(drawGoochPipelineContext.getPipeline())->getRenderpassProperties(), additionalProps);

    clearQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    clearQuadPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassRt);
    clearQuadPipelineContext.swapchainIdx = 0;
    clearQuadPipelineContext.materialName = "ClearRT";
    vulkanRenderingContext->preparePipelineContext(&clearQuadPipelineContext);

    RenderPassAdditionalProps renderPassAdditionalProps;
    renderPassAdditionalProps.bUsedAsPresentSource = true;

    drawQuadPipelineContext.bUseSwapchainFb = true;
    drawQuadPipelineContext.materialName = "DrawQuadFromTexture";
    drawQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawQuadPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&drawQuadPipelineContext);
    drawQuadRenderPass = vulkanRenderingContext->getRenderPass(
        static_cast<const GraphicsPipelineBase*>(drawQuadPipelineContext.getPipeline())->getRenderpassProperties(), renderPassAdditionalProps);
}

void ExperimentalEngine::createPipelineResources()
{
    VkClearValue baseClearValue;
    baseClearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
    baseClearValue.depthStencil.depth = 0;
    baseClearValue.depthStencil.stencil = 0;
    smAttachmentsClearColors.resize(drawSmPipelineContext.getFb()->textures.size(), baseClearValue);
    swapchainClearColor = baseClearValue;

    ENQUEUE_COMMAND(QuadVerticesInit,LAMBDA_BODY
        (
            const std::array<Vector3D, 3> quadVerts = { Vector3D(-1,-1,0),Vector3D(3,-1,0),Vector3D(-1,3,0) };
            const std::array<uint32, 3> quadIndices = { 0,1,2 };// 3 Per tri of quad

            quadVertexBuffer = new GraphicsVertexBuffer(sizeof(Vector3D), static_cast<uint32>(quadVerts.size()));
            quadVertexBuffer->setResourceName("ScreenQuadVertices");
            quadVertexBuffer->init();
            quadIndexBuffer = new GraphicsIndexBuffer(sizeof(uint32), static_cast<uint32>(quadIndices.size()));
            quadIndexBuffer->setResourceName("ScreenQuadIndices");
            quadIndexBuffer->init();

            cmdList->copyToBuffer(quadVertexBuffer, 0, quadVerts.data(), uint32(quadVertexBuffer->getResourceSize()));
            cmdList->copyToBuffer(quadIndexBuffer, 0, quadIndices.data(), uint32(quadIndexBuffer->getResourceSize()));
        )
        , this);

    // Shader pipeline's buffers and image access
    createShaderParameters();
}

void ExperimentalEngine::destroyPipelineResources()
{
    ENQUEUE_COMMAND(QuadVerticesRelease,LAMBDA_BODY
        (
            quadVertexBuffer->release();
            quadIndexBuffer->release();
            delete quadVertexBuffer;
            quadVertexBuffer = nullptr;
            delete quadIndexBuffer;
            quadIndexBuffer = nullptr;
        )
    , this);
    // Shader pipeline's buffers and image access
    destroyShaderParameters();
}

void ExperimentalEngine::updateCameraParams()
{
    ViewData viewDataTemp;

    if (appInstance().inputSystem()->isKeyPressed(Keys::RMB))
    {
        cameraRotation.yaw() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseX)->currentValue * timeData.deltaTime * timeData.activeTimeDilation * 15.0f;
        cameraRotation.pitch() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseY)->currentValue * timeData.deltaTime * timeData.activeTimeDilation * 15.0f;
    }

    if (appInstance().inputSystem()->isKeyPressed(Keys::A))
    {
        cameraTranslation -= cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::D))
    {
        cameraTranslation += cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::W))
    {
        cameraTranslation += cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::S))
    {
        cameraTranslation -= cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::Q))
    {
        cameraTranslation -= Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::E))
    {
        cameraTranslation += Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->keyState(Keys::P)->keyWentUp)
    {
        camera.cameraProjection = camera.cameraProjection == ECameraProjection::Perspective ? ECameraProjection::Orthographic : ECameraProjection::Perspective;
        viewDataTemp.projection = camera.projectionMatrix();
        viewDataTemp.invProjection = viewDataTemp.projection.inverse();

        viewParameters->setMatrixParam("projection", viewDataTemp.projection);
        viewParameters->setMatrixParam("invProjection", viewDataTemp.invProjection);
        lightCommon->setMatrixParam("projection", viewDataTemp.projection);
        lightCommon->setMatrixParam("invProjection", viewDataTemp.invProjection);
    }
    if (appInstance().inputSystem()->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = Rotation();
    }

    camera.setRotation(cameraRotation);
    camera.setTranslation(cameraTranslation);

    viewDataTemp.view = camera.viewMatrix();
    viewDataTemp.invView = viewDataTemp.view.inverse();
    viewParameters->setMatrixParam("view", viewDataTemp.view);
    viewParameters->setMatrixParam("invView", viewDataTemp.invView);
    lightCommon->setMatrixParam("view", viewDataTemp.view);
    lightCommon->setMatrixParam("invView", viewDataTemp.invView);
}

void ExperimentalEngine::onStartUp()
{
    GameEngine::onStartUp();

    ENQUEUE_COMMAND(EngineStartUp, { startUpRenderInit(); }, this);

    camera.cameraProjection = ECameraProjection::Perspective;
    camera.setOrthoSize({ 1280,720 });
    camera.setClippingPlane(0.1f, 5000.f);
    camera.setFOV(110.f, 90.f);

    cameraTranslation = Vector3D(0.f, 1.f, 0.0f).safeNormalize() * (500);
    cameraTranslation.z() += 200;

    camera.setTranslation(cameraTranslation);
    camera.lookAt(Vector3D::ZERO);
    cameraRotation = camera.rotation();

    createScene();

    tempTest();
}

void ExperimentalEngine::startUpRenderInit()
{
    vDevice = VulkanGraphicsHelper::getVulkanDevice(getRenderApi()->getGraphicsInstance());
    device = VulkanGraphicsHelper::getDevice(vDevice);
    graphicsDbg = VulkanGraphicsHelper::debugGraphics(getRenderApi()->getGraphicsInstance());
    createPools();
    frameResources.resize(getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount());

    createFrameResources();
    getPipelineForSubpass();
    createImages();
    createPipelineResources();
    setupShaderParameterParams();
}

void ExperimentalEngine::onQuit()
{
    ENQUEUE_COMMAND(EngineQuit, { renderQuit(); }, this);

    GameEngine::onQuit();
}

void ExperimentalEngine::renderQuit()
{
    vDevice->vkDeviceWaitIdle(device);

    destroyPipelineResources();
    destroyFrameResources();

    destroyImages();

    destroyScene();

    destroyPools();
}

void ExperimentalEngine::frameRender()
{
    VkViewport viewport;
    viewport.x = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    viewport.width = float(EngineSettings::screenSize.get().x);
    // Since view matrix positive y is along up while vulkan positive y in view is down
    viewport.height = -(float(EngineSettings::screenSize.get().y));
    viewport.y = float(EngineSettings::screenSize.get().y);
    VkRect2D scissor = { {0,0},{EngineSettings::screenSize.get().x,EngineSettings::screenSize.get().y} };

    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    drawSmPipelineContext.swapchainIdx = drawQuadPipelineContext.swapchainIdx = index;
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawSmPipelineContext);
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawQuadPipelineContext);

    drawGoochPipelineContext.rtTextures[0] = frameResources[index].lightingPassRt;
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawGoochPipelineContext);

    GraphicsPipelineQueryParams queryParam;
    queryParam.cullingMode = ECullingMode::BackFace;
    queryParam.drawMode = EPolygonDrawMode::Fill;

    SharedPtr<ShaderParameters> drawQuadDescs;
    switch (frameVisualizeId)
    {
    case 1:
        drawQuadDescs = drawQuadTextureDescs[index];
        break;
    case 2:
        drawQuadDescs = drawQuadNormalDescs[index];
        break;
    case 3:
        drawQuadDescs = drawQuadDepthDescs[index];
        break;
    case 0:
    default:
        drawQuadDescs = drawLitColorsDescs[index];
        break;
    }


    if (!frameResources[index].recordingFence->isSignaled())
    {
        frameResources[index].recordingFence->waitForSignal();
    }
    frameResources[index].recordingFence->resetSignal();

    CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(frameResources[index].perFrameCommands, &cmdBeginInfo);
    {
        const GraphicsPipeline* tempPipeline = static_cast<const GraphicsPipeline*>(drawSmPipelineContext.getPipeline());

        SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ExperimentalEngineFrame);

        RENDERPASS_BEGIN_INFO(renderPassBeginInfo);
        renderPassBeginInfo.renderPass = drawSmRenderPass;
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(GBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, index));

        renderPassBeginInfo.pClearValues = smAttachmentsClearColors.data();
        renderPassBeginInfo.clearValueCount = (uint32)smAttachmentsClearColors.size();
        renderPassBeginInfo.renderArea = scissor;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, MainUnlitPass);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            //vDevice->vkCmdPushConstants(frameResources[index].perFrameCommands, tempPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &useVertexColor);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, tempPipeline->getPipeline(queryParam));
            // View set
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                , tempPipeline->pipelineLayout, 0, 1, &static_cast<const VulkanShaderSetParameters*>(viewParameters.get())->descriptorsSet, 0, nullptr);

            for (const SceneEntity& entity : sceneData)
            {
                // Instance set
                vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                    , tempPipeline->pipelineLayout, 1, 1, &static_cast<const VulkanShaderSetParameters*>(entity.instanceParameters.get())->descriptorsSet, 0, nullptr);

                uint64 vertexBufferOffset = 0;
                vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(entity.meshAsset->vertexBuffer)->buffer, &vertexBufferOffset);
                vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(entity.meshAsset->indexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

                uint32 meshBatchIdx = 0;
                for (const MeshVertexView& meshBatch : entity.meshAsset->meshBatches)
                {
                    // Batch set
                    vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                        , tempPipeline->pipelineLayout, 2, 1, &static_cast<const VulkanShaderSetParameters*>(entity.meshBatchParameters[meshBatchIdx].get())->descriptorsSet, 0, nullptr);

                    vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, meshBatch.numOfIndices, 1, meshBatch.startIndex, 0, 0);

                    ++meshBatchIdx;
                }
            }
        }
        vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);
        // Drawing lighting quads
        tempPipeline = static_cast<const GraphicsPipeline*>(drawGoochPipelineContext.getPipeline());
        viewport.height = float(EngineSettings::screenSize.get().y);
        viewport.y = 0;

        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = smAttachmentsClearColors.data();
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(drawGoochPipelineContext.getFb());
        renderPassBeginInfo.renderArea = scissor;
        renderPassBeginInfo.renderPass = lightingRenderPass;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ClearLightingRTs);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            uint64 vertexBufferOffset = 0;
            vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, &vertexBufferOffset);
            vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

            // Clear resolve first
            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                , static_cast<const GraphicsPipeline*>(clearQuadPipelineContext.getPipeline())->getPipeline(queryParam));
            for (const std::pair<const uint32, VkDescriptorSet>& descriptorsSet : static_cast<VulkanShaderParameters*>(clearInfoParams.get())->descriptorsSets)
            {
                vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                    , static_cast<const GraphicsPipeline*>(clearQuadPipelineContext.getPipeline())->pipelineLayout, descriptorsSet.first, 1, &descriptorsSet.second, 0, nullptr);
            }
            vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, 3, 1, 0, 0, 0);
        }
        vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);

        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, LightingPass);

            // TODO(Jeslas) Change lighting to array of lights per pass
            for (const std::pair<const GoochModelLightData, SharedPtr<ShaderParameters>>& light : lightData)
            {
                vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
                vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
                vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

                uint64 vertexBufferOffset = 0;
                vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, &vertexBufferOffset);
                vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

                vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, tempPipeline->getPipeline(queryParam));

                VulkanShaderParameters* lightCommonParams = static_cast<VulkanShaderParameters*>(lightCommon.get());
                VulkanShaderParameters* lightFrameParams = static_cast<VulkanShaderParameters*>(lightTextures[index].get());

                // Right now only one set will be there but there is chances more set might get added
                for (const std::pair<const uint32, VkDescriptorSet>& descriptorsSet : lightCommonParams->descriptorsSets)
                {
                    vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                        , tempPipeline->pipelineLayout, descriptorsSet.first, 1, &descriptorsSet.second, 0, nullptr);
                }
                for (const std::pair<const uint32, VkDescriptorSet>& descriptorsSet : lightFrameParams->descriptorsSets)
                {
                    vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                        , tempPipeline->pipelineLayout, descriptorsSet.first, 1, &descriptorsSet.second, 0, nullptr);
                }
                for (const std::pair<const uint32, VkDescriptorSet>& descriptorsSet : static_cast<VulkanShaderParameters*>(light.second.get())->descriptorsSets)
                {
                    vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                        , tempPipeline->pipelineLayout, descriptorsSet.first, 1, &descriptorsSet.second, 0, nullptr);
                }
                vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, 3, 1, 0, 0, 0);
                vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);
            }
        }

        // Drawing final quad
        tempPipeline = static_cast<const GraphicsPipeline*>(drawQuadPipelineContext.getPipeline());

        viewport.x = 0;
        viewport.y = 0;
        viewport.width = float(EngineSettings::surfaceSize.get().x);
        viewport.height = float(EngineSettings::surfaceSize.get().y);
        scissor = { {0,0},{EngineSettings::surfaceSize.get().x,EngineSettings::surfaceSize.get().y} };

        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &swapchainClearColor;
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(GBuffers::getSwapchainFramebuffer(index));
        renderPassBeginInfo.renderArea = scissor;
        renderPassBeginInfo.renderPass = drawQuadRenderPass;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ResolveToSwapchain);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, tempPipeline->getPipeline(queryParam));

            for (const std::pair<const uint32, VkDescriptorSet>& descriptorsSet : static_cast<VulkanShaderParameters*>(drawQuadDescs.get())->descriptorsSets)
            {
                vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                    , tempPipeline->pipelineLayout, descriptorsSet.first, 1, &descriptorsSet.second, 0, nullptr);
            }

            uint64 vertexBufferOffset = 0;
            vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, &vertexBufferOffset);
            vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

            vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, 3, 1, 0, 0, 0);
        }
        vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);
    }
    vDevice->vkEndCommandBuffer(frameResources[index].perFrameCommands);

    VkPipelineStageFlags flag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    SUBMIT_INFO(qSubmitInfo);
    qSubmitInfo.commandBufferCount = 1;
    qSubmitInfo.pCommandBuffers = &frameResources[index].perFrameCommands;
    qSubmitInfo.waitSemaphoreCount = 1;
    qSubmitInfo.pWaitDstStageMask = &flag;
    qSubmitInfo.pWaitSemaphores = &static_cast<VulkanSemaphore*>(waitSemaphore.get())->semaphore;
    qSubmitInfo.signalSemaphoreCount = 1;
    qSubmitInfo.pSignalSemaphores = &static_cast<VulkanSemaphore*>(frameResources[index].usageWaitSemaphore[0].get())->semaphore;

    vDevice->vkQueueSubmit(getQueue<EQueueFunction::Graphics>(vDevice)->getQueueOfPriority<EQueuePriority::High>()
        , 1, &qSubmitInfo, static_cast<VulkanFence*>(frameResources[index].recordingFence.get())->fence);

    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };
    GraphicsHelper::presentImage(getRenderApi()->getGraphicsInstance(), &canvases, &indices, &frameResources[index].usageWaitSemaphore);

}

void ExperimentalEngine::tickEngine()
{
    GameEngine::tickEngine();
    updateCameraParams();

    if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::ONE))
    {
        frameVisualizeId = 0;
    }
    else if(getApplicationInstance()->inputSystem()->isKeyPressed(Keys::TWO))
    {
        frameVisualizeId = 1;
    }
    else if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::THREE))
    {
        frameVisualizeId = 2;
    }
    else if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::FOUR))
    {
        frameVisualizeId = 3;
    }

    if (appInstance().inputSystem()->keyState(Keys::X)->keyWentUp)
    {
        toggleRes = !toggleRes;
        ENQUEUE_COMMAND(WritingDescs,
            {
                const Size2D& screenSize = toggleRes ? EngineSettings::surfaceSize.get() : Size2D(1280, 720);
                GBuffers::onScreenResized(screenSize);
                resizeLightingRts(screenSize);
                reupdateTextureParamsOnResize();
                EngineSettings::screenSize.set(screenSize);                
            }, this);
    }

    ENQUEUE_COMMAND(TickFrame,
        {
            updateShaderParameters(cmdList, graphicsInstance);
            frameRender(); 
        }, this);

    tempTestPerFrame();
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    static ExperimentalEngine gameEngine;
    return &gameEngine;
}