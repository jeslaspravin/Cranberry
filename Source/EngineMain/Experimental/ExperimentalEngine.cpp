#include "ExperimentalEngine.h"

#if EXPERIMENTAL

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
#include "../RenderInterface/Rendering/CommandBuffer.h"
#include "ImGui/ImGuiManager.h"
#include "ImGui/imgui.h"

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
    lightTextures.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadTextureDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadNormalDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadDepthDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawLitColorsDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));

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
        lightTextures.set(GraphicsHelper::createShaderParameters(graphicsInstance, goochModelDescLayout, { 0,2 }), i);
        lightTextures.getResources()[i]->setResourceName("LightFrameCommon_" + iString);
        drawQuadTextureDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadTextureDescs.getResources()[i]->setResourceName("QuadUnlit_" + iString);
        drawQuadNormalDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadNormalDescs.getResources()[i]->setResourceName("QuadNormal_" + iString);
        drawQuadDepthDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadDepthDescs.getResources()[i]->setResourceName("QuadDepth_" + iString);
        drawLitColorsDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawLitColorsDescs.getResources()[i]->setResourceName("QuadLit_" + iString);
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
        lightTextures.getResources()[i]->setTextureParam("ssUnlitColor", multibuffer->textures[1], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssNormal", multibuffer->textures[3], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssDepth", multibuffer->textures[5], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssColor", frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering);

        drawQuadTextureDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[1], linearFiltering);
        drawQuadNormalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[3], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[5], linearFiltering);
        drawLitColorsDescs.getResources()[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
    }
    lightTextures.init();
    drawQuadTextureDescs.init();
    drawQuadNormalDescs.init();
    drawQuadDepthDescs.init();
    drawLitColorsDescs.init();

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

        std::vector<GraphicsResource*> shaderParams;
        ShaderParameters::staticType()->allRegisteredResources(shaderParams, true, true);
        for (GraphicsResource* resource : shaderParams)
        {
            static_cast<ShaderParameters*>(resource)->updateParams(cmdList, graphicsInstance);
        }
    }
}

void ExperimentalEngine::reupdateTextureParamsOnResize()
{
    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* multibuffer = GBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, i);
        lightTextures.getResources()[i]->setTextureParam("ssUnlitColor", multibuffer->textures[1], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssNormal", multibuffer->textures[3], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssDepth", multibuffer->textures[5], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssColor", frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering);

        drawQuadTextureDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[1], linearFiltering);
        drawQuadNormalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[3], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[5], linearFiltering);
        drawLitColorsDescs.getResources()[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
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

    lightTextures.reset();
    drawQuadTextureDescs.reset();
    drawQuadNormalDescs.reset();
    drawQuadDepthDescs.reset();
    drawLitColorsDescs.reset();

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
        frameResources[i].lightingPassResolved->setTextureSize(size);
        getRenderApi()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
        getRenderApi()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassResolved });
    }
}

void ExperimentalEngine::createFrameResources()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    RenderTextureCreateParams rtCreateParams;
    rtCreateParams.bSameReadWriteTexture = true;
    rtCreateParams.filtering = ESamplerFiltering::Linear;
    rtCreateParams.format = ERenderTargetFormat::RT_U8;
    rtCreateParams.sampleCount = EPixelSampleCount::SampleCount1;
    rtCreateParams.textureSize = EngineSettings::screenSize.get();

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        String name = "Frame";
        name.append(std::to_string(i));

        frameResources[i].usageWaitSemaphore.push_back(GraphicsHelper::createSemaphore(getRenderApi()->getGraphicsInstance(), (name + "QueueSubmit").c_str()));
        frameResources[i].recordingFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), (name + "RecordingGaurd").c_str(),true);

        rtCreateParams.textureName = "LightingRT_" + std::to_string(i);
        frameResources[i].lightingPassRt = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
        rtCreateParams.textureName = "LightingResolved_" + std::to_string(i);
        frameResources[i].lightingPassResolved = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
    }
}

void ExperimentalEngine::destroyFrameResources()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        frameResources[i].usageWaitSemaphore[0]->release();
        frameResources[i].recordingFence->release();
        frameResources[i].usageWaitSemaphore[0].reset();
        frameResources[i].recordingFence.reset();

        getRenderApi()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassRt);
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassResolved);
    }
}

void ExperimentalEngine::getPipelineForSubpass()
{
    VulkanGlobalRenderingContext* vulkanRenderingContext = static_cast<VulkanGlobalRenderingContext*>(getRenderApi()->getGlobalRenderingContext());

    drawSmPipelineContext.forVertexType = EVertexType::StaticMesh;
    drawSmPipelineContext.materialName = "SingleColor";
    drawSmPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffers;
    drawSmPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&drawSmPipelineContext);

    // Gooch model
    drawGoochPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawGoochPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassRt);
    drawGoochPipelineContext.materialName = "GoochModel";
    vulkanRenderingContext->preparePipelineContext(&drawGoochPipelineContext);
    lightingRenderPass = vulkanRenderingContext->getRenderPass(
        static_cast<const GraphicsPipelineBase*>(drawGoochPipelineContext.getPipeline())->getRenderpassProperties(), {});

    clearQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    clearQuadPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassResolved);
    clearQuadPipelineContext.materialName = "ClearRT";
    vulkanRenderingContext->preparePipelineContext(&clearQuadPipelineContext);
    
    resolveLightRtPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    resolveLightRtPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassResolved);
    resolveLightRtPipelineContext.materialName = "DrawQuadFromTexture";
    vulkanRenderingContext->preparePipelineContext(&resolveLightRtPipelineContext);

    drawQuadPipelineContext.bUseSwapchainFb = true;
    drawQuadPipelineContext.materialName = "DrawQuadFromTexture";
    drawQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawQuadPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&drawQuadPipelineContext);
}

void ExperimentalEngine::createPipelineResources()
{
    clearValues.colors.resize(drawSmPipelineContext.getFb()->textures.size(), LinearColorConst::BLACK);

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

    getRenderApi()->getImGuiManager()->addLayer(this);
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

    getRenderApi()->getImGuiManager()->removeLayer(this);
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

void ExperimentalEngine::frameRender(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    drawSmPipelineContext.swapchainIdx = drawQuadPipelineContext.swapchainIdx = index;
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawSmPipelineContext);
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawQuadPipelineContext);

    drawGoochPipelineContext.rtTextures[0] = frameResources[index].lightingPassRt;
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawGoochPipelineContext);
    resolveLightRtPipelineContext.rtTextures[0] = frameResources[index].lightingPassResolved;
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&resolveLightRtPipelineContext);

    GraphicsPipelineQueryParams queryParam;
    queryParam.cullingMode = ECullingMode::BackFace;
    queryParam.drawMode = EPolygonDrawMode::Fill;

    ShaderParameters* drawQuadDescs;
    switch (frameVisualizeId)
    {
    case 1:
        drawQuadDescs = *drawQuadTextureDescs;
        break;
    case 2:
        drawQuadDescs = *drawQuadNormalDescs;
        break;
    case 3:
        drawQuadDescs = *drawQuadDepthDescs;
        break;
    case 0:
    default:
        drawQuadDescs = *drawLitColorsDescs;
        break;
    }


    if (!frameResources[index].recordingFence->isSignaled())
    {
        frameResources[index].recordingFence->waitForSignal();
    }
    frameResources[index].recordingFence->resetSignal();

    QuantizedBox2D viewport;
    // Since view matrix positive y is along up while vulkan positive y in view is down
    viewport.minBound.x = 0;
    viewport.minBound.y = EngineSettings::screenSize.get().y;
    viewport.maxBound.x = EngineSettings::screenSize.get().x;
    viewport.maxBound.y = 0;

    QuantizedBox2D scissor;
    scissor.minBound = { 0, 0 };
    scissor.maxBound = EngineSettings::screenSize.get();

    String cmdName = "FrameRender" + std::to_string(index);
    cmdList->finishCmd(cmdName);
    const GraphicsResource* cmdBuffer = cmdList->startCmd(cmdName, EQueueFunction::Graphics, true);
    VkCommandBuffer frameCmdBuffer = VulkanGraphicsHelper::getRawCmdBuffer(graphicsInstance, cmdBuffer);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ExperimentalEngineFrame);
        cmdList->cmdBeginRenderPass(cmdBuffer, drawSmPipelineContext, scissor, {}, clearValues);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, MainUnlitPass);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

            //vDevice->vkCmdPushConstants(frameResources[index].perFrameCommands, tempPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &useVertexColor);
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawSmPipelineContext, { queryParam });

            // View set
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawSmPipelineContext, viewParameters.get());
            for (const SceneEntity& entity : sceneData)
            {
                // Instance set
                cmdList->cmdBindDescriptorsSets(cmdBuffer, drawSmPipelineContext, entity.instanceParameters.get());

                cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { entity.meshAsset->vertexBuffer }, { 0 });
                cmdList->cmdBindIndexBuffer(cmdBuffer, entity.meshAsset->indexBuffer);

                uint32 meshBatchIdx = 0;
                for (const MeshVertexView& meshBatch : entity.meshAsset->meshBatches)
                {
                    // Batch set
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, drawSmPipelineContext, entity.meshBatchParameters[meshBatchIdx].get());
                    cmdList->cmdDrawIndexed(cmdBuffer, meshBatch.startIndex, meshBatch.numOfIndices);
                    ++meshBatchIdx;
                }
            }
        }
        cmdList->cmdEndRenderPass(cmdBuffer);

        // Drawing lighting quads
        viewport.minBound = Int2D(0, 0);
        viewport.maxBound = EngineSettings::screenSize.get();

        cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { quadVertexBuffer }, { 0 });
        cmdList->cmdBindIndexBuffer(cmdBuffer, quadIndexBuffer);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBeginRenderPass(cmdBuffer, resolveLightRtPipelineContext, scissor, {}, clearValues);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, ClearLightingRTs);

            // Clear resolve first
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, clearQuadPipelineContext, { queryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, clearQuadPipelineContext, clearInfoParams.get());
            cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);

        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, LightingPass);

            // TODO(Jeslas) Change lighting to array of lights per pass
            int32 lightIndex = 0;
            for (const std::pair<const GoochModelLightData, SharedPtr<ShaderParameters>>& light : lightData)
            {
                cmdList->cmdBeginRenderPass(cmdBuffer, drawGoochPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawLight);
                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawGoochPipelineContext, { queryParam });

                    // Right now only one set will be there but there is chances more set might get added
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, drawGoochPipelineContext, { lightCommon.get(), *lightTextures, light.second.get() });
                    cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
                }
                cmdList->cmdEndRenderPass(cmdBuffer);

                lightIndex++;

                if (lightIndex < lightData.size())
                {
                    cmdList->cmdBeginRenderPass(cmdBuffer, resolveLightRtPipelineContext, scissor, {}, clearValues);
                    {
                        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveLightRT);

                        cmdList->cmdBindGraphicsPipeline(cmdBuffer, resolveLightRtPipelineContext, { queryParam });
                        cmdList->cmdBindDescriptorsSets(cmdBuffer, resolveLightRtPipelineContext, *drawLitColorsDescs);

                        cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
                    }
                    cmdList->cmdEndRenderPass(cmdBuffer);
                }                
            }
        }

        // Drawing IMGUI
        TinyDrawingContext drawingContext;
        drawingContext.cmdBuffer = cmdBuffer;
        drawingContext.rtTextures = drawGoochPipelineContext.rtTextures;
        getRenderApi()->getImGuiManager()->draw(cmdList, graphicsInstance, drawingContext);

        // Drawing final quad        
        viewport.maxBound = scissor.maxBound = EngineSettings::surfaceSize.get();

        cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { quadVertexBuffer }, { 0 });
        cmdList->cmdBindIndexBuffer(cmdBuffer, quadIndexBuffer);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        RenderPassAdditionalProps renderPassAdditionalProps;
        renderPassAdditionalProps.bUsedAsPresentSource = true;
        cmdList->cmdBeginRenderPass(cmdBuffer, drawQuadPipelineContext, scissor, renderPassAdditionalProps, clearValues);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveToSwapchain);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawQuadPipelineContext, { queryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawQuadPipelineContext, drawQuadDescs);
            cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.waitOn = { CommandSubmitInfo::WaitInfo{ waitSemaphore.get(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT } };
    submitInfo.signalSemaphores = { frameResources[index].usageWaitSemaphore[0].get() };
    submitInfo.cmdBuffers = { cmdBuffer };

    cmdList->submitCmd(EQueuePriority::High, submitInfo, frameResources[index].recordingFence);

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
            frameRender(cmdList, graphicsInstance); 
        }, this);

    tempTestPerFrame();
}

int32 ExperimentalEngine::layerDepth() const
{
    return 0;
}

int32 ExperimentalEngine::sublayerDepth() const
{
    return 0;
}

void ExperimentalEngine::draw(class ImGuiDrawInterface* drawInterface)
{
    bool bOpen = true;
    ImGui::ShowDemoWindow(&bOpen);
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    static ExperimentalEngine gameEngine;
    return &gameEngine;
}
#endif