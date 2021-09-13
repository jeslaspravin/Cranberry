#include "../Core/Engine/GameEngine.h"

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
#include "../Core/Types/Textures/Texture2D.h"
#include "../Core/Types/Textures/TexturesBase.h"
#include "../RenderApi/GBuffersAndTextures.h"
#include "../Core/Input/Keys.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../VulkanRI/VulkanGraphicsHelper.h"
#include "../RenderApi/RenderManager.h"
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
#include "../Editor/Core/ImGui/ImGuiManager.h"
#include "../Editor/Core/ImGui/ImGuiLib/imgui.h"
#include "../Editor/Core/ImGui/ImGuiLib/implot.h"
#include "../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../RenderInterface/Resources/QueueResource.h"
#include "../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSyncResource.h"
#include "../RenderInterface/Resources/MemoryResources.h"
#include "../RenderInterface/ShaderCore/ShaderParameters.h"
#include "../Core/Platform/LFS/PlatformLFS.h"
#include "../Core/Types/Transform3D.h"
#include "../Core/Types/Camera/Camera.h"
#include "../Core/Types/Colors.h"
#include "../RenderInterface/Rendering/RenderingContexts.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../RenderInterface/Resources/BufferedResources.h"
#include "../Editor/Core/ImGui/IImGuiLayer.h"

#include <array>
#include <random>
#include <map>

#include <vulkan_core.h>

struct AOS
{
    Vector4D a;
    Vector2D b;
    Vector2D c[4];
};

class ShaderParameters;
class IRenderCommandList;
class Texture2DRW;

struct TestBitonicSortIndices
{
    struct LineSegment
    {
        std::array<float, 2> step;
        std::array<float, 2> indices;
    };

    int32 count;
    int32 stepsCount = 0;
    std::vector<std::pair<std::vector<LineSegment>, Color>> perThreadIndices;
    std::vector<std::pair<std::vector<LineSegment>, Color>> perGroup;

    TestBitonicSortIndices(int32 n)
        : count(n)
    {
        int32 flipsNum = int32(Math::ceil(Math::log2(count)));
        count = Math::pow(2, flipsNum);
        stepsCount = (flipsNum * (2 + (flipsNum - 1))) / 2;

        perGroup.resize(count);
        for (std::pair<std::vector<LineSegment>, Color>& color : perGroup)
        {
            color.second = ColorConst::random();
        }

        int32 threadNum = count / 2;
        for (int32 t = 0; t < threadNum; t++)
        {
            std::pair<std::vector<LineSegment>, Color>& threadIndices = perThreadIndices.emplace_back();
            threadIndices.second = ColorConst::random();

            float step = 0;
            for (int32 h = 2; h <= count; h *= 2)
            {
                step++;

                int32 flipStartIdx = (t / (h / 2)) * h;
                int32 flipOffset = (t % (h / 2));

                int32 flipLhsIdx = flipStartIdx + flipOffset;
                int32 flipRhsIdx = flipStartIdx + h - flipOffset - 1;

                LineSegment* segment = &threadIndices.first.emplace_back();
                segment->step[0] = segment->step[1] = step + (flipOffset / (h * 0.5f));
                segment->indices[0] = float(flipLhsIdx);
                segment->indices[1] = float(flipRhsIdx);

                perGroup[flipStartIdx].first.emplace_back(*segment);

                for (int32 d = h / 2; d >= 2; d /= 2)
                {
                    step++;

                    int32 disperseStartIdx = (t / (d / 2)) * d;
                    int32 disperseOffset = (t % (d / 2));

                    int32 dLhsIdx = disperseStartIdx + disperseOffset;
                    int32 dRhsIdx = dLhsIdx + (d / 2);

                    segment = &threadIndices.first.emplace_back();
                    segment->step[0] = segment->step[1] = step + (disperseOffset / (d * 0.5f));
                    segment->indices[0] = float(dLhsIdx);
                    segment->indices[1] = float(dRhsIdx);

                    perGroup[flipStartIdx].first.emplace_back(*segment);  
                }
            }

            stepsCount = Math::max(stepsCount, int32(Math::ceil(step)));
        }
    }
};

struct QueueCommandPool
{
    VkCommandPool tempCommandsPool;
    VkCommandPool resetableCommandPool;
    VkCommandPool oneTimeRecordPool;
};

struct TexelBuffer
{
    class BufferResource* buffer = nullptr;
    // Only necessary for texel buffers
    VkBufferView bufferView = nullptr;
};

struct ImageData
{
    class TextureBase* image = nullptr;
    VkImageView imageView = nullptr;
};

struct SceneEntity
{
    Transform3D transform;
    class StaticMeshAsset* meshAsset;

    SharedPtr<ShaderParameters> instanceParameters;
    std::vector<LinearColor> meshBatchColors;
    std::vector<SharedPtr<ShaderParameters>> meshBatchParameters;
};

struct FrameResource
{
    std::vector<SharedPtr<GraphicsSemaphore>> usageWaitSemaphore;
    RenderTargetTexture* lightingPassRt;
    RenderTargetTexture* lightingPassResolved;
    SharedPtr<GraphicsFence> recordingFence;
};

class ExperimentalEngine : public GameEngine, public IImGuiLayer
{
    class VulkanDevice* vDevice;
    VkDevice device;
    const class VulkanDebugGraphics* graphicsDbg;

    std::map<EQueueFunction, QueueCommandPool> pools;
    void createPools();
    void destroyPools();

    SharedPtr<class SamplerInterface> nearestFiltering = nullptr;
    SharedPtr<class SamplerInterface> linearFiltering = nullptr;
    // TODO(Jeslas) : Cubic filtering not working check new drivers or log bug in nvidia
    // SharedPtr<class SamplerInterface> cubicFiltering = nullptr;
    void createImages();
    void destroyImages();

    // Scene data
    std::vector<SceneEntity> sceneData;
    std::vector<struct GoochModelLightData> sceneLightData;
    std::vector<SharedPtr<ShaderParameters>> lightData;
    SharedPtr<ShaderParameters> lightCommon;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> lightTextures;
    SharedPtr<ShaderParameters> viewParameters;
    void createScene();
    void destroyScene();

    // Camera parameters
    Camera camera;
    Vector3D cameraTranslation;
    Rotation cameraRotation;
    void updateCameraParams();

    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadTextureDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadNormalDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadDepthDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawLitColorsDescs;

    void createShaderParameters();
    void setupShaderParameterParams();
    void updateShaderParameters(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    void destroyShaderParameters();

    void resizeLightingRts(const Size2D& size);
    void reupdateTextureParamsOnResize();

    // Shader pipeline resources
    RenderPassClearValue clearValues;

    void createFrameResources();
    void destroyFrameResources();

    LocalPipelineContext drawSmPipelineContext;

    VkRenderPass lightingRenderPass;
    LocalPipelineContext drawGoochPipelineContext;

    class BufferResource* quadVertexBuffer = nullptr;
    class BufferResource* quadIndexBuffer = nullptr;
    LocalPipelineContext drawQuadPipelineContext;
    LocalPipelineContext resolveLightRtPipelineContext;

    SharedPtr<ShaderParameters> clearInfoParams;
    LocalPipelineContext clearQuadPipelineContext;

    ImageData writeTexture;
    SharedPtr<ShaderParameters> testComputeParams;
    LocalPipelineContext testComputePipelineContext;

    void getPipelineForSubpass();

    std::vector<FrameResource> frameResources;
    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources

    bool bAnimateX;
    bool bAnimateY;
    int32 frameVisualizeId = 0;// 0 color 1 normal 2 depth
    Size2D renderSize{ 1280, 720 };
    ECameraProjection projection = ECameraProjection::Perspective;
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

    void startUpRenderInit();
    void renderQuit();
    void frameRender(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);

    void tempTest();
    void tempTestPerFrame();
    /* IImGuiLayer Implementation */
public:
    int32 layerDepth() const override;
    int32 sublayerDepth() const override;
    void draw(class ImGuiDrawInterface* drawInterface) override;
    /* end overrides */
};

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
    nearestFiltering = GraphicsHelper::createSampler(gEngine->getRenderManager()->getGraphicsInstance(), "NearestSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Nearest);
    linearFiltering = GraphicsHelper::createSampler(gEngine->getRenderManager()->getGraphicsInstance(), "LinearSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Linear);

    Texture2DRWCreateParams createParam;
    createParam.textureSize = Size2D(512, 512);
    createParam.mipCount = 1;
    createParam.textureName = "Compute Write";
    createParam.format = EPixelDataFormat::RGBA_U8_Norm;
    createParam.bIsWriteOnly = false;
    writeTexture.image = TextureBase::createTexture<Texture2DRW>(createParam);
    writeTexture.imageView = static_cast<VulkanImageResource*>(writeTexture.image->getTextureResource())->getImageView({});
}

void ExperimentalEngine::destroyImages()
{
    TextureBase::destroyTexture<Texture2DRW>(writeTexture.image);
    writeTexture.image = nullptr;
    nearestFiltering->release();
    linearFiltering->release();
}

void ExperimentalEngine::createScene()
{
    StaticMeshAsset* cube = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cube.obj"));
    StaticMeshAsset* sphere = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Sphere.obj"));
    StaticMeshAsset* cylinder = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cylinder.obj"));
    StaticMeshAsset* cone = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cone.obj"));
    std::array<StaticMeshAsset*, 4> assets{ cube, sphere, cylinder, cone };
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    std::normal_distribution<float> distribution1(0.0, 1.0);

    for (int32 i = 0; i < 3; i++)
    {
        for (int32 j = 0; j < 3; j++)
        {
            Vector3D offset = Vector3D(i * 1200.0f, j * 1200.0f, 0);
            SceneEntity sceneFloor;
            sceneFloor.meshAsset = cube;
            sceneFloor.transform.setScale(Vector3D(10, 10, 1));
            sceneFloor.transform.setTranslation(offset + Vector3D(0, 0, -50));
            sceneFloor.meshBatchColors.emplace_back(LinearColorConst::random());

            sceneData.emplace_back(sceneFloor);

            // Ceiling
            sceneFloor.transform.setTranslation(offset + Vector3D(0, 0, 550));
            sceneData.emplace_back(sceneFloor);

            // Pillars
            sceneFloor.meshAsset = cylinder;
            sceneFloor.transform.setScale(Vector3D(1, 1, 5));
            sceneFloor.transform.setTranslation(offset + Vector3D(450, 450, 250));
            sceneData.emplace_back(sceneFloor);
            sceneFloor.transform.setTranslation(offset + Vector3D(-450, 450, 250));
            sceneData.emplace_back(sceneFloor);
            sceneFloor.transform.setTranslation(offset + Vector3D(450, -450, 250));
            sceneData.emplace_back(sceneFloor);
            sceneFloor.transform.setTranslation(offset + Vector3D(-450, -450, 250));
            sceneData.emplace_back(sceneFloor);

            for (uint32 i = 0; i < 5; ++i)
            {
                SceneEntity entity;
                entity.meshAsset = assets[std::rand() % assets.size()];
                entity.transform.setTranslation(offset + Vector3D(distribution(generator) * 400, distribution(generator) * 400, distribution1(generator) * 100 + 50));
                entity.transform.setRotation(Rotation(0, 0, distribution(generator) * 45));

                entity.meshBatchColors.emplace_back(LinearColorConst::random());
                sceneData.emplace_back(entity);
            }

            GoochModelLightData light;
            light.warmOffsetAndPosX = Vector4D(0.3f, 0.3f, 0.0f, offset.x() + 0);
            light.coolOffsetAndPosY = Vector4D(0.0f, 0.0f, 0.55f, offset.y() + 0);

            // Near floor
            float height = 150;

            // Middle light
            light.highlightColorAndPosZ = Vector4D(1.f, 1.f, 1.f, offset.z() + height);
            light.lightColorAndRadius = Vector4D(1.f, 1.f, 1.f, 0);
            sceneLightData.emplace_back(light);

            // Light 1
            light.highlightColorAndPosZ = Vector4D(0.49f, 0.66f, 0.75f, offset.z() + height);
            light.lightColorAndRadius = Vector4D(0.45f, 0.58f, 0.80f, 0);

            light.warmOffsetAndPosX.w() = offset.x() + 400;
            light.coolOffsetAndPosY.w() = offset.y() + 400;
            sceneLightData.emplace_back(light);
            // Light 2
            light.coolOffsetAndPosY.w() = -light.coolOffsetAndPosY.w();
            sceneLightData.emplace_back(light);
            // Light 3
            light.warmOffsetAndPosX.w() = -light.warmOffsetAndPosX.w();
            sceneLightData.emplace_back(light);
            // Light 4
            light.coolOffsetAndPosY.w() = -light.coolOffsetAndPosY.w();
            sceneLightData.emplace_back(light);
        }
    }
}

void ExperimentalEngine::destroyScene()
{
    sceneData.clear();
}

void ExperimentalEngine::createShaderParameters()
{
    IGraphicsInstance* graphicsInstance = getRenderManager()->getGraphicsInstance();
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
    const uint32 lightDataCount = uint32(Math::ceil(sceneLightData.size() / float(ARRAY_LENGTH(GoochModelLightArray::lights))));
    lightData.resize(lightDataCount);
    for (uint32 i = 0; i < lightDataCount; ++i)
    {
        // as 0 and 1 are light common and textures
        lightData[i] = GraphicsHelper::createShaderParameters(graphicsInstance, goochModelDescLayout, { 0, 1 });
        lightData[i]->setResourceName("Light_" + std::to_string(i * ARRAY_LENGTH(GoochModelLightArray::lights)) + "to"
            + std::to_string(i * ARRAY_LENGTH(GoochModelLightArray::lights) + ARRAY_LENGTH(GoochModelLightArray::lights)));
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

    testComputeParams = GraphicsHelper::createShaderParameters(graphicsInstance, testComputePipelineContext.getPipeline()->getParamLayoutAtSet(0));
    testComputeParams->setResourceName("TestCompute");
}

void ExperimentalEngine::setupShaderParameterParams()
{
    IGraphicsInstance* graphicsInstance = getRenderManager()->getGraphicsInstance();

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
    lightCommon->setIntParam("lightsCount", uint32(sceneLightData.size()));
    lightCommon->setFloatParam("invLightsCount", 1.0f / sceneLightData.size());
    lightCommon->init();
    uint32 lightStartIdx = 0;
    for (SharedPtr<ShaderParameters>& light : lightData)
    {
        uint32 rangeIdx = 0;
        for (; rangeIdx < ARRAY_LENGTH(GoochModelLightArray::lights) && (rangeIdx + lightStartIdx) < sceneLightData.size(); ++rangeIdx)
        {
            light->setBuffer("lights", sceneLightData[rangeIdx + lightStartIdx], rangeIdx);
        }
        light->setIntParam("count", uint32(rangeIdx));
        light->init();

        lightStartIdx += ARRAY_LENGTH(GoochModelLightArray::lights);
    }

    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    ImageViewInfo depthImageViewInfo;
    depthImageViewInfo.componentMapping.g = depthImageViewInfo.componentMapping.b 
        = depthImageViewInfo.componentMapping.a = depthImageViewInfo.componentMapping.r = EPixelComponentMapping::R;
    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* multibuffer = GlobalBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, i);
        const int32 fbIncrement = multibuffer->bHasResolves ? 2 : 1;
        const int32 resolveIdxOffset = multibuffer->bHasResolves ? 1 : 0;
        lightTextures.getResources()[i]->setTextureParam("ssUnlitColor", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssNormal", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssDepth", multibuffer->textures[(3 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParamViewInfo("ssDepth", depthImageViewInfo);
        lightTextures.getResources()[i]->setTextureParam("ssColor", frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering);

        drawQuadTextureDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadNormalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(3 * fbIncrement)], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParamViewInfo("quadTexture", depthImageViewInfo);
        drawLitColorsDescs.getResources()[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
    }
    lightTextures.init();
    drawQuadTextureDescs.init();
    drawQuadNormalDescs.init();
    drawQuadDepthDescs.init();
    drawLitColorsDescs.init();

    clearInfoParams->setVector4Param("clearColor", Vector4D(0, 0, 0, 0));
    clearInfoParams->init();

    testComputeParams->setTextureParam("resultImage", writeTexture.image->getTextureResource());
    testComputeParams->resizeRuntimeBuffer("inData", 2);
    AOS testRuntime;
    testRuntime.a = Vector4D(1, 0, 1, 0);
    testRuntime.b = Vector2D::FWD;
    testRuntime.c[0] = Vector2D::RIGHT;
    testRuntime.c[1] = Vector2D::FWD;
    testRuntime.c[2] = Vector2D::RIGHT;
    testRuntime.c[3] = Vector2D::FWD;
    testComputeParams->setVector4Param("test1", testRuntime.a);
    testComputeParams->setBuffer("data", testRuntime, 0);
    testComputeParams->setBuffer("data", testRuntime, 1);
    testComputeParams->init();
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
        Framebuffer* multibuffer = GlobalBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, i);
        const int32 fbIncrement = multibuffer->bHasResolves ? 2 : 1;
        const int32 resolveIdxOffset = multibuffer->bHasResolves ? 1 : 0;
        lightTextures.getResources()[i]->setTextureParam("ssUnlitColor", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssNormal", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssDepth", multibuffer->textures[(3 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssColor", frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering);

        drawQuadTextureDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadNormalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(3 * fbIncrement)], linearFiltering);
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

    for (SharedPtr<ShaderParameters>& light : lightData)
    {
        light->release();
        light.reset();
    }

    lightTextures.reset();
    drawQuadTextureDescs.reset();
    drawQuadNormalDescs.reset();
    drawQuadDepthDescs.reset();
    drawLitColorsDescs.reset();

    clearInfoParams->release();
    clearInfoParams.reset();

    testComputeParams->release();
    testComputeParams.reset();
}

void ExperimentalEngine::resizeLightingRts(const Size2D& size)
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        frameResources[i].lightingPassRt->setTextureSize(size);
        frameResources[i].lightingPassResolved->setTextureSize(size);
        getRenderManager()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
        getRenderManager()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassResolved });
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

        frameResources[i].usageWaitSemaphore.push_back(GraphicsHelper::createSemaphore(getRenderManager()->getGraphicsInstance(), (name + "QueueSubmit").c_str()));
        frameResources[i].recordingFence = GraphicsHelper::createFence(getRenderManager()->getGraphicsInstance(), (name + "RecordingGaurd").c_str(),true);

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

        getRenderManager()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassRt);
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassResolved);
    }
}

void ExperimentalEngine::getPipelineForSubpass()
{
    VulkanGlobalRenderingContext* vulkanRenderingContext = static_cast<VulkanGlobalRenderingContext*>(getRenderManager()->getGlobalRenderingContext());

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

    testComputePipelineContext.materialName = "TestCompute";
    vulkanRenderingContext->preparePipelineContext(&testComputePipelineContext);
}

void ExperimentalEngine::createPipelineResources()
{
    clearValues.colors.resize(drawSmPipelineContext.getFb()->textures.size(), LinearColorConst::BLACK);

    ENQUEUE_COMMAND_NODEBUG(QuadVerticesInit,LAMBDA_BODY
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
    ENQUEUE_COMMAND_NODEBUG(QuadVerticesRelease,LAMBDA_BODY
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
        cameraRotation.yaw() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseX)->currentValue * timeData.activeTimeDilation * 0.25f;
        cameraRotation.pitch() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseY)->currentValue * timeData.activeTimeDilation * 0.25f;
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
    if (appInstance().inputSystem()->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = RotationMatrix::fromZX(Vector3D::UP, cameraRotation.fwdVector()).asRotation();
    }

    if (camera.cameraProjection != projection)
    {
        camera.cameraProjection = projection;
        viewDataTemp.projection = camera.projectionMatrix();
        viewDataTemp.invProjection = viewDataTemp.projection.inverse();

        viewParameters->setMatrixParam("projection", viewDataTemp.projection);
        viewParameters->setMatrixParam("invProjection", viewDataTemp.invProjection);
        lightCommon->setMatrixParam("projection", viewDataTemp.projection);
        lightCommon->setMatrixParam("invProjection", viewDataTemp.invProjection);
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

    ENQUEUE_COMMAND_NODEBUG(EngineStartUp, { startUpRenderInit(); }, this);

    camera.cameraProjection = projection;
    camera.setOrthoSize({ 1280,720 });
    camera.setClippingPlane(0.1f, 6000.f);
    camera.setFOV(110.f, 90.f);

    cameraTranslation = Vector3D(0.f, -1.f, 0.0f).safeNormalize() * (500);
    cameraTranslation.z() += 200;

    camera.setTranslation(cameraTranslation);
    camera.lookAt(Vector3D::ZERO);
    cameraRotation = camera.rotation();

    getRenderManager()->getImGuiManager()->addLayer(this);
    createScene();

    tempTest();
}

void ExperimentalEngine::startUpRenderInit()
{
    vDevice = VulkanGraphicsHelper::getVulkanDevice(getRenderManager()->getGraphicsInstance());
    device = VulkanGraphicsHelper::getDevice(vDevice);
    graphicsDbg = VulkanGraphicsHelper::debugGraphics(getRenderManager()->getGraphicsInstance());
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
    ENQUEUE_COMMAND_NODEBUG(EngineQuit, { renderQuit(); }, this);

    getRenderManager()->getImGuiManager()->removeLayer(this);
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
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&drawSmPipelineContext);
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&drawQuadPipelineContext);

    drawGoochPipelineContext.rtTextures[0] = frameResources[index].lightingPassRt;
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&drawGoochPipelineContext);
    resolveLightRtPipelineContext.rtTextures[0] = frameResources[index].lightingPassResolved;
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&resolveLightRtPipelineContext);

    GraphicsPipelineQueryParams queryParam;
    queryParam.cullingMode = ECullingMode::BackFace;
    queryParam.drawMode = EPolygonDrawMode::Fill;


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

    //{
    //    cmdList->finishCmd(cmdName + "_Comp");
    //    auto temp = cmdList->startCmd(cmdName + "_Comp", EQueueFunction::Compute, true);
    //    cmdList->cmdBindComputePipeline(temp, testComputePipelineContext);
    //    cmdList->cmdBindDescriptorsSets(temp, testComputePipelineContext, { testComputeParams.get() });
    //    cmdList->cmdBarrierResources(temp, { testComputeParams.get() });
    //    cmdList->cmdDispatch(temp
    //        , writeTexture.image->getTextureSize().x / static_cast<const ComputeShader*>(testComputePipelineContext.getPipeline()->getShaderResource())->getSubGroupSize().x
    //        , writeTexture.image->getTextureSize().y / static_cast<const ComputeShader*>(testComputePipelineContext.getPipeline()->getShaderResource())->getSubGroupSize().y
    //    );
    //    cmdList->endCmd(temp);

    //    CommandSubmitInfo2 cs2;
    //    cs2.cmdBuffers = { temp };
    //    cmdList->submitCmd(EQueuePriority::High, cs2);
    //}
    const GraphicsResource* cmdBuffer = cmdList->startCmd(cmdName, EQueueFunction::Graphics, true);
    VkCommandBuffer frameCmdBuffer = VulkanGraphicsHelper::getRawCmdBuffer(graphicsInstance, cmdBuffer);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ExperimentalEngineFrame);
        cmdList->cmdBindComputePipeline(cmdBuffer, testComputePipelineContext);

        std::vector<std::pair<String, std::any>> pushConsts = {
            {"time" , { Time::asSeconds(Time::timeNow())} }
            , {"flags", { uint32((bAnimateX ? 0x00000001 : 0) | (bAnimateY ? 0x00000010 : 0)) }}
        };
        cmdList->cmdPushConstants(cmdBuffer, testComputePipelineContext, pushConsts);
        cmdList->cmdBindDescriptorsSets(cmdBuffer, testComputePipelineContext, { testComputeParams.get() });
        cmdList->cmdBarrierResources(cmdBuffer, { testComputeParams.get() });
        cmdList->cmdDispatch(cmdBuffer
            , writeTexture.image->getTextureSize().x / static_cast<const ComputeShader*>(testComputePipelineContext.getPipeline()->getShaderResource())->getSubGroupSize().x
            , writeTexture.image->getTextureSize().y / static_cast<const ComputeShader*>(testComputePipelineContext.getPipeline()->getShaderResource())->getSubGroupSize().y
        );

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
        if(frameVisualizeId == 0)
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, LightingPass);

            cmdList->cmdBeginRenderPass(cmdBuffer, resolveLightRtPipelineContext, scissor, {}, clearValues);
            {
                SCOPED_CMD_MARKER(cmdList, cmdBuffer, ClearLightingRTs);

                // Clear resolve first
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, clearQuadPipelineContext, { queryParam });
                cmdList->cmdBindDescriptorsSets(cmdBuffer, clearQuadPipelineContext, clearInfoParams.get());
                cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
            }
            cmdList->cmdEndRenderPass(cmdBuffer);

            int32 lightDataIndex = 0;
            for (const SharedPtr<ShaderParameters>& light : lightData)
            {
                cmdList->cmdBeginRenderPass(cmdBuffer, drawGoochPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawLight);
                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawGoochPipelineContext, { queryParam });

                    cmdList->cmdBindDescriptorsSets(cmdBuffer, drawGoochPipelineContext, { lightCommon.get(), *lightTextures, light.get() });
                    cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
                }
                cmdList->cmdEndRenderPass(cmdBuffer);

                lightDataIndex++;

                if (lightDataIndex < lightData.size())
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
        else
        {
            ShaderParameters* drawQuadDescs = nullptr;
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
                break;
            }

            if (drawQuadDescs)
            {
                resolveLightRtPipelineContext.rtTextures = drawGoochPipelineContext.rtTextures;
                getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&resolveLightRtPipelineContext);

                cmdList->cmdBeginRenderPass(cmdBuffer, resolveLightRtPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveFrame);

                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, resolveLightRtPipelineContext, { queryParam });
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, resolveLightRtPipelineContext, drawQuadDescs);

                    cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
                }
                cmdList->cmdEndRenderPass(cmdBuffer);
            }
        }

        // Drawing IMGUI
        TinyDrawingContext drawingContext;
        drawingContext.cmdBuffer = cmdBuffer;
        drawingContext.rtTextures = drawGoochPipelineContext.rtTextures;
        getRenderManager()->getImGuiManager()->draw(cmdList, graphicsInstance, drawingContext);

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
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawQuadPipelineContext, *drawLitColorsDescs);
            cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.waitOn = { CommandSubmitInfo::WaitInfo{ waitSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT } };
    submitInfo.signalSemaphores = { frameResources[index].usageWaitSemaphore[0] };
    submitInfo.cmdBuffers = { cmdBuffer };

    cmdList->submitCmd(EQueuePriority::High, submitInfo, frameResources[index].recordingFence);

    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };
    cmdList->presentImage(canvases, indices, {});
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

    if (renderSize != EngineSettings::screenSize.get())
    {
        ENQUEUE_COMMAND_NODEBUG(WritingDescs,
            {
                GlobalBuffers::onScreenResized(renderSize);
                resizeLightingRts(renderSize);
                reupdateTextureParamsOnResize();
                EngineSettings::screenSize.set(renderSize);
            }, this);
    }

    ENQUEUE_COMMAND_NODEBUG(TickFrame,
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
    static bool bOpen = false;
    if (bOpen)
    {
        ImGui::ShowDemoWindow(&bOpen);
    }

    static bool bTestOpen = true;

    if (bTestOpen)
    {
        ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Test", &bTestOpen, ImGuiWindowFlags_NoMove))
        {
            ImGui::End();
            return;
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            const InputAnalogState* rmxState = getApplicationInstance()->inputSystem()->analogState(AnalogStates::RelMouseX);
            const InputAnalogState* rmyState = getApplicationInstance()->inputSystem()->analogState(AnalogStates::RelMouseY);
            const InputAnalogState* amxState = getApplicationInstance()->inputSystem()->analogState(AnalogStates::AbsMouseX);
            const InputAnalogState* amyState = getApplicationInstance()->inputSystem()->analogState(AnalogStates::AbsMouseY);
            ImGui::Text("Cursor pos (%.0f, %.0f) Delta (%0.1f, %0.1f)", amxState->currentValue, amyState->currentValue, rmxState->currentValue, rmyState->currentValue);

            if (ImGui::CollapsingHeader("Camera"))
            {
                ImGui::Columns(2);
                ImGui::Text("Projection");
                ImGui::NextColumn();
                {
                    const char* proj[] = { "Perspective", "Orthographic" };
                    static int currVal = 0;
                    ImGui::Combo("Projection", &currVal, proj, ARRAY_LENGTH(proj));
                    switch (currVal)
                    {
                    case 0:
                        projection = ECameraProjection::Perspective;
                        break;
                    case 1:
                        projection = ECameraProjection::Orthographic;
                        break;
                    }
                }
            }

            ImGui::Columns(1);
            ImGui::NextColumn();
            if (ImGui::CollapsingHeader("Rendering"))
            {
                ImGui::Columns(2);
                ImGui::Text("Render Size");
                ImGui::NextColumn();
                {
                    const char* resolutions[] = { "1280x720", "1920x1080", "2560x1440", "3840x2160" };
                    static int currRes = 0;
                    ImGui::Combo("Size", &currRes, resolutions, ARRAY_LENGTH(resolutions));
                    switch (currRes)
                    {
                    case 0:
                        renderSize = Size2D(1280, 720);
                        break;
                    case 1:
                        renderSize = Size2D(1920, 1080);
                        break;
                    case 2:
                        renderSize = Size2D(2560, 1440);
                        break;
                    case 3:
                        renderSize = Size2D(3840, 2160);
                        break;
                    }
                }

                ImGui::NextColumn();
                ImGui::Text("Visualize buffer");
                ImGui::NextColumn();
                {
                    const char* bufferMode[] = { "Lit", "Unlit", "Normal", "Depth" };
                    ImGui::Combo("Frame", &frameVisualizeId, bufferMode, ARRAY_LENGTH(bufferMode));
                }
            };

            ImGui::Columns(1);
            ImGui::NextColumn();
            if (ImGui::CollapsingHeader("Compute"))
            {
                ImGui::Text("Animate");
                ImGui::NextColumn();
                ImGui::Checkbox("X", &bAnimateX); ImGui::SameLine();
                ImGui::Checkbox("Y", &bAnimateY);
                ImGui::NextColumn();
                ImGui::Text("%f", Time::asSeconds(Time::timeNow()));

                ImGui::Separator();
                ImGui::NextColumn();
                ImGui::Image(writeTexture.image, ImVec2(
                    ImGui::GetWindowContentRegionWidth(),
                    ImGui::GetWindowContentRegionWidth()));
                ImGui::Separator();
            }

            if (ImGui::CollapsingHeader("Bitonic Sort"))
            {
                static TestBitonicSortIndices bitonic(16);
                if (ImGui::InputInt("Count", &bitonic.count))
                {
                    bitonic = TestBitonicSortIndices(bitonic.count);
                }

                ImPlot::SetNextPlotLimits(0, bitonic.stepsCount + 1, -1, bitonic.count, ImGuiCond_::ImGuiCond_Always);
                if (ImPlot::BeginPlot("Bitonic Threads", 0, 0, ImVec2(-1, 0), ImPlotFlags_::ImPlotFlags_CanvasOnly
                    , ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_NoGridLines
                    , ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_Invert))
                {
                    int32 idx = 0;
                    for (const auto& threadInds : bitonic.perThreadIndices)
                    {
                        String labelId = "Thread: " + std::to_string(idx);
                        ImPlot::PushStyleColor(ImPlotCol_::ImPlotCol_Line, LinearColor(threadInds.second));
                        int32 segIdx = 0;
                        for (const TestBitonicSortIndices::LineSegment& seg : threadInds.first)
                        {
                            String segId = labelId + "Segment : " + std::to_string(segIdx);
                            ImPlot::PlotLine(segId.getChar(), seg.step.data(), seg.indices.data(), int32(seg.indices.size()));
                            segIdx++;
                        }
                        ImPlot::PopStyleColor();
                        idx++;
                    }
                    ImPlot::EndPlot();
                }

                ImPlot::SetNextPlotLimits(0, bitonic.stepsCount + 1, -1, bitonic.count, ImGuiCond_::ImGuiCond_Always);
                if (ImPlot::BeginPlot("Bitonic Groups", 0, 0, ImVec2(-1, 0), ImPlotFlags_::ImPlotFlags_CanvasOnly
                    , ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_NoGridLines
                    , ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_Invert))
                {
                    int32 idx = 0;
                    for (const auto& grpInds : bitonic.perGroup)
                    {
                        String labelId = "Group: " + std::to_string(idx);
                        ImPlot::PushStyleColor(ImPlotCol_::ImPlotCol_Line, LinearColor(grpInds.second));
                        int32 segIdx = 0;
                        for (const TestBitonicSortIndices::LineSegment& seg : grpInds.first)
                        {
                            String segId = labelId + "Segment : " + std::to_string(segIdx);
                            ImPlot::PlotLine(segId.getChar(), seg.step.data(), seg.indices.data(), int32(seg.indices.size()));
                            segIdx++;
                        }
                        ImPlot::PopStyleColor();
                        idx++;
                    }

                    ImPlot::EndPlot();
                }
            }

            ImGui::PopStyleVar();
            ImGui::End();
        }
    }
}

//GameEngine* GameEngineWrapper::createEngineInstance()
//{
//    static ExperimentalEngine gameEngine;
//    return &gameEngine;
//}
#endif