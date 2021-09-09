#include "../Core/Engine/GameEngine.h"
#if EXPERIMENTAL

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
#include "../Core/Math/BVH.h"
#include "../Core/Types/Textures/RenderTargetTextures.h"
#include "../RenderInterface/GlobalRenderVariables.h"
#include "../RenderInterface/Rendering/CommandBuffer.h"
#include "../Editor/Core/ImGui/ImGuiManager.h"
#include "../Editor/Core/ImGui/ImGuiLib/imgui.h"
#include "../Editor/Core/ImGui/ImGuiLib/implot.h"
#include "../Core/Engine/WindowManager.h"
#include "../Core/Platform/GenericAppWindow.h"
#include "../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../RenderInterface/Shaders/EngineShaders/PBRShader.h"
#include "../Core/Types/Textures/ImageUtils.h"
#include "../Assets/Asset/EnvironmentMapAsset.h"

#include <array>
#include <random>
#include <map>

#include <vulkan_core.h>

struct QueueCommandPool
{
    VkCommandPool tempCommandsPool;
    VkCommandPool resetableCommandPool;
    VkCommandPool oneTimeRecordPool;
};

struct PBRSceneEntity
{
    struct BatchProperties
    {
        LinearColor color;
        float roughness;
        float metallic;
        Vector2D uvScale{ Vector2D::ONE };
        String textureName;
        LocalPipelineContext* pipeline;
    };
    Transform3D transform;
    class StaticMeshAsset* meshAsset;
    String name;
    std::vector<BatchProperties> meshBatchProps;

    // Generated
    SharedPtr<ShaderParameters> instanceParameters;
    std::vector<SharedPtr<ShaderParameters>> meshBatchParameters;
};

struct FrameResource
{
    std::vector<SharedPtr<GraphicsSemaphore>> usageWaitSemaphore;
    RenderTargetTexture* lightingPassRt;
    RenderTargetTexture* lightingPassResolved;
    SharedPtr<GraphicsFence> recordingFence;
};


struct PointLight
{
    Vector3D lightPos;
    LinearColor lightcolor;
    float radius;
    float lumen;
    String name;

    SharedPtr<ShaderParameters> paramCollection;
    uint32 index;

    void update() const;
};

struct SpotLight
{
    Transform3D transform;
    LinearColor lightcolor;
    float radius;
    float lumen;
    float innerCone;
    float outerCone;
    String name;

    SharedPtr<ShaderParameters> paramCollection;
    uint32 index;

    void update() const;
};

struct DirectionalLight
{
    Rotation direction;
    LinearColor lightcolor;
    float lumen;

    SharedPtr<ShaderParameters> paramCollection;

    void update() const;
};

struct GridEntity
{
    enum Type
    {
        Invalid,
        Entity,
        SpotLight,
        PointLight,
    };

    Type type = Invalid;
    uint32 idx;

    AABB getBounds() const;

    bool operator==(const GridEntity& other) const
    {
        return type == other.type && idx == other.idx;
    }
};

class ExperimentalEnginePBR : public GameEngine, public IImGuiLayer
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
    std::vector<PBRSceneEntity> sceneData;

    std::vector<SpotLight> sceneSpotLights;
    std::vector<PointLight> scenePointLights;
    DirectionalLight dirLight;

    BoundingVolume<GridEntity> sceneVolume;
    GridEntity selection;

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
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadAmbientDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadRoughDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadMetalDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawQuadDepthDescs;
    SwapchainBufferedResource<SharedPtr<ShaderParameters>> drawLitColorsDescs;

    void createShaderParameters();
    void setupShaderParameterParams();
    void updateShaderParameters(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    void destroyShaderParameters();

    void resizeLightingRts(const Size2D& size);
    void reupdateTextureParamsOnResize();
    void reupdateEnvMap();

    // Shader pipeline resources
    RenderPassClearValue clearValues;

    void createFrameResources();
    void destroyFrameResources();

    LocalPipelineContext singleColorPipelineContext;
    LocalPipelineContext texturedPipelineContext;

    LocalPipelineContext drawPbrPipelineContext;

    LocalPipelineContext resolveToPresentPipelineContext;
    LocalPipelineContext overBlendedQuadPipelineContext;
    LocalPipelineContext resolveLightRtPipelineContext;

    SharedPtr<ShaderParameters> clearInfoParams;
    LocalPipelineContext clearQuadPipelineContext;

    LocalPipelineContext sceneDebugLinesPipelineContext;

    LocalPipelineContext drawLinesDWritePipelineCntxt;
    // Gizmo drawing
    RenderTargetTexture* camGizmoColorTexture;
    RenderTargetTexture* camGizmoDepthTarget;
    void updateCamGizmoViewParams();
    SharedPtr<ShaderParameters> camViewAndInstanceParams;
    SharedPtr<ShaderParameters> camRTParams;

    void getPipelineForSubpass();

    std::vector<FrameResource> frameResources;
    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources

    float exposure = 4.2f;
    float gamma = 2.2f;
    bool bDrawTbn = false;
    int32 frameVisualizeId = 0;// 0 color 1 normal 2 depth
    Size2D renderSize{ 1280, 720 };
    ECameraProjection projection = ECameraProjection::Perspective;

    // Textures
    std::vector<TextureAsset*> textures;
    std::vector<EnvironmentMapAsset*> envMaps;

    // Histogram data
    std::vector<const AChar*> textureNames;
    int32 selectedTexture = 0;
    std::array<float, 32> histogram[3];

    // Env texture
    std::vector<const AChar*> envMapNames;
    int32 selectedEnv = 0;

    String noneString{ "None" };
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

    void startUpRenderInit();
    void renderQuit();
    void updateCamGizmoCapture(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    void frameRender(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    void debugFrameRender(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsResource* cmdBuffer, uint32 swapchainIdx);

    void drawSelectionWidget(class ImGuiDrawInterface* drawInterface);

    void tempTest();
    void tempTestPerFrame();
    /* IImGuiLayer Implementation */
public:
    int32 layerDepth() const override;
    int32 sublayerDepth() const override;
    void draw(class ImGuiDrawInterface* drawInterface) override;
    /* end overrides */

    AABB getBounds(const GridEntity& entity) const
    {
        switch (entity.type)
        {
        case GridEntity::Entity:
        {
            fatalAssert(sceneData.size() > entity.idx, "%s() : Invalid index %d", __func__, entity.idx);
            AABB bound(sceneData[entity.idx].meshAsset->bounds.minBound * sceneData[entity.idx].transform.getScale() + sceneData[entity.idx].transform.getTranslation(),
                sceneData[entity.idx].meshAsset->bounds.maxBound * sceneData[entity.idx].transform.getScale() + sceneData[entity.idx].transform.getTranslation());
            return bound;
        }
        case GridEntity::PointLight:
        {
            fatalAssert(scenePointLights.size() > entity.idx, "%s() : Invalid index %d", __func__, entity.idx);
            AABB bound(scenePointLights[entity.idx].lightPos - Vector3D(50), scenePointLights[entity.idx].lightPos + Vector3D(50));
            return bound;
        }
        case GridEntity::SpotLight:
        {
            fatalAssert(sceneSpotLights.size() > entity.idx, "%s() : Invalid index %d", __func__, entity.idx);
            AABB bound(sceneSpotLights[entity.idx].transform.getTranslation() - Vector3D(50), sceneSpotLights[entity.idx].transform.getTranslation() + Vector3D(50));
            return bound;
        }
        default:
            fatalAssert(false, "%s(): Unsupported type", __func__);
            break;
        }
        return { Vector3D::ZERO, Vector3D::ZERO };
    }
};

AABB GridEntity::getBounds() const
{
    return static_cast<ExperimentalEnginePBR*>(*gEngine)->getBounds(*this);
}

void ExperimentalEnginePBR::tempTest()
{
    
}

void ExperimentalEnginePBR::tempTestPerFrame()
{

}

template <EQueueFunction QueueFunction> VulkanQueueResource<QueueFunction>* getQueue(const VulkanDevice* device);

void ExperimentalEnginePBR::createPools()
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

void ExperimentalEnginePBR::destroyPools()
{
    for (const std::pair<const EQueueFunction, QueueCommandPool>& pool : pools)
    {
        vDevice->vkDestroyCommandPool(device, pool.second.oneTimeRecordPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.resetableCommandPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.tempCommandsPool, nullptr);
    }
}

void ExperimentalEnginePBR::createImages()
{
    nearestFiltering = GraphicsHelper::createSampler(gEngine->getRenderManager()->getGraphicsInstance(), "NearestSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Nearest, float(EngineSettings::minSamplingMipLevel.get()));
    linearFiltering = GraphicsHelper::createSampler(gEngine->getRenderManager()->getGraphicsInstance(), "LinearSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Linear, float(EngineSettings::minSamplingMipLevel.get()));

    RenderTextureCreateParams rtCreateParams;
    rtCreateParams.bSameReadWriteTexture = true;
    rtCreateParams.bIsSrgb = false;
    rtCreateParams.format = ERenderTargetFormat::RT_U8Packed;
    rtCreateParams.textureSize = Size2D(256, 256);
    camGizmoColorTexture = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);

    rtCreateParams.format = ERenderTargetFormat::RT_Depth;
    camGizmoDepthTarget = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
}

void ExperimentalEnginePBR::destroyImages()
{
    nearestFiltering->release();
    linearFiltering->release();

    TextureBase::destroyTexture<RenderTargetTexture>(camGizmoColorTexture);
    TextureBase::destroyTexture<RenderTargetTexture>(camGizmoDepthTarget);
}

void ExperimentalEnginePBR::createScene()
{
    StaticMeshAsset* cube = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cube.obj"));
    //StaticMeshAsset* plane = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Plane.obj"));
    StaticMeshAsset* sphere = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Sphere.obj"));
    StaticMeshAsset* cylinder = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cylinder.obj"));
    StaticMeshAsset* cone = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Cone.obj"));
    StaticMeshAsset* suzanne = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset("Suzanne.obj"));
    std::array<StaticMeshAsset*, 5> assets{ cube, sphere, cylinder, cone, suzanne };
#if NDEBUG
    std::array<String, 8> floorTypes{ "WoodFloor043", "Tiles086", "Tiles074", "MetalPlates006", "Marble006", "Ground042", "Ground037", "Gravel022" };
    std::array<String, 6> ceilTypes{ "WoodFloor043", "Tiles108", "Tiles074", "MetalPlates006", "Marble006", "Wood051" };
    std::array<String, 9> pillarTypes{ "WoodFloor043", "Tiles108", "Tiles074", "MetalPlates006", "Marble006", "Marble006", "Rock035", "Ground037", "PaintedPlaster016" };
    std::array<String, 15> textures{ "Bricks065", "Gravel022", "Ground037", "Ground042", "Leather028", "Marble006", "Metal034", "Metal038", "MetalPlates006"
        , "PaintedPlaster016", "Rock035","Tiles086", "Tiles074" , "Tiles108", "Wood051" };
#else
    std::array<String, 1> floorTypes{ "Tiles074" };
    std::array<String, 1> ceilTypes{ "Tiles074" };
    std::array<String, 1> pillarTypes{ "Tiles074" };
    std::array<String, 1> textures{ "Tiles074" };
#endif

    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    std::uniform_real_distribution<float> ud01(0.0, 1.0);
    std::normal_distribution<float> distribution1(0.5f, 0.15f);

    const Vector2D floorTextureScale(1 / 16.0f);
    const Vector2D pillarTextureScale(1 / 3.0f, 1 / 6.0f);
    const Vector2D textureScale(1 / 3.0f);

    std::list<GridEntity> entities;
    auto pushEntity = [&entities, this](const PBRSceneEntity& entity)
    {
        entities.emplace_back(GridEntity{ GridEntity::Entity, uint32(sceneData.size()) });
        sceneData.emplace_back(entity);
    };
    auto pushSpt = [&entities, this](const SpotLight& spotLight)
    {
        entities.emplace_back(GridEntity{ GridEntity::SpotLight, uint32(sceneSpotLights.size()) });
        sceneSpotLights.emplace_back(spotLight);
    };
    auto pushPt = [&entities, this](const PointLight& pointLight)
    {
        entities.emplace_back(GridEntity{ GridEntity::PointLight, uint32(scenePointLights.size()) });
        scenePointLights.emplace_back(pointLight);
    };

    for (int32 i = -1; i <= 1; i++)
    {
        for (int32 j = -1; j <= 1; j++)
        {
            String roomIdx = std::to_string((i + 1) * 3 + j + 1);
            Vector3D offset = Vector3D(i * 1400.0f, j * 1400.0f, 0);
            PBRSceneEntity sceneFloor;
            sceneFloor.meshAsset = cube;
            sceneFloor.transform.setScale(Vector3D(13, 13, 1));
            sceneFloor.transform.setTranslation(offset + Vector3D(0, 0, -50));
            sceneFloor.name = "floor" + roomIdx;

            for (uint32 batchIdx = 0; batchIdx < sceneFloor.meshAsset->meshBatches.size(); ++batchIdx)
            {
                sceneFloor.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties
                {
                    //{ ud01(generator) * 0.75f, ud01(generator) * 0.75f, ud01(generator) * 0.75f, 1 }
                    LinearColorConst::WHITE
                    , 1.f, 1.f
                    , floorTextureScale
                    , floorTypes[uint32(floorTypes.size() * ud01(generator))]
                    , &texturedPipelineContext
                });
            }
            pushEntity(sceneFloor);

            dirLight.direction.pitch() = 35;
            dirLight.direction.yaw() = 45;
            dirLight.lumen = 20;
            dirLight.lightcolor = LinearColor(1.f, 1.f, 0.8f);

            if (i == 0 && j == 0)
            {
                for (uint32 m = 0; m < 10; m++)
                {
                    for (uint32 r = 0; r < 10; r++)
                    {
                        float rough = (r * 0.1f) + 0.05f;
                        float metallic = (m * 0.1f) + 0.05f;
                        String suffix = "_R_" + std::to_string(r)
                            + "_M_" + std::to_string(m);

                        Vector3D pos = offset + Vector3D(65.f + m * 130.0f, 65.f + r * 130.f, 25.f) - Vector3D(650, 650, 0);

                        PBRSceneEntity entity;
                        entity.transform.setTranslation(pos + Vector3D(0, 0, 75));
                        entity.meshAsset = sphere;
                        entity.name = sphere->assetName() + suffix;

                        for (uint32 batchIdx = 0; batchIdx < entity.meshAsset->meshBatches.size(); ++batchIdx)
                        {
                            entity.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties
                            {
                                { 0.5f, 0.0f, 0.0f }
                                ,  rough, metallic
                                , textureScale
                                , textures[uint32(textures.size() * ud01(generator))]
                                , &singleColorPipelineContext
                            });
                        }
                        pushEntity(entity);

                        entity.meshAsset = cube;
                        entity.name = cube->assetName() + suffix;
                        entity.meshBatchProps.clear();
                        for (uint32 batchIdx = 0; batchIdx < entity.meshAsset->meshBatches.size(); ++batchIdx)
                        {
                            entity.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties
                                {
                                    { 0.5f, 0.0f, 0.0f }
                                    ,  rough, metallic
                                    , textureScale
                                    , textures[uint32(textures.size() * ud01(generator))]
                                    , &singleColorPipelineContext
                                });
                        }
                        entity.transform.setTranslation(pos);
                        entity.transform.setScale(Vector3D(1, 1, 0.5));
                        pushEntity(entity);
                    }
                }
                // Lights
                {
                    PointLight light;
                    light.radius = 700;
                    light.lumen = 250;
                    light.lightcolor = LinearColorConst::WHITE;

                    light.lightPos = offset + Vector3D(250, 250, 250);
                    light.name = "point0_" + roomIdx;
                    pushPt(light);

                    light.lightPos = offset + Vector3D(250, -250, 250);
                    light.name = "point1_" + roomIdx;
                    pushPt(light);

                    light.lightPos = offset + Vector3D(-250, 250, 250);
                    light.name = "point2_" + roomIdx;
                    pushPt(light);

                    light.lightPos = offset + Vector3D(-250, -250, 250);
                    light.name = "point3_" + roomIdx;
                    pushPt(light);
                }
            }
            else
            {
                // Ceiling
                for (auto& batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = ceilTypes[uint32(ceilTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(0, 0, 550));
                sceneFloor.name = "ceil" + roomIdx;
                pushEntity(sceneFloor);

                for (uint32 i = 0; i < 5; ++i)
                {
                    PBRSceneEntity entity;
                    entity.meshAsset = assets[std::rand() % assets.size()];
                    entity.transform.setTranslation(offset + Vector3D(distribution(generator) * 400, distribution(generator) * 400, distribution1(generator) * 100 + 50));
                    entity.transform.setRotation(Rotation(0, 0, distribution(generator) * 45));
                    entity.name = entity.meshAsset->assetName() + roomIdx + "_" + std::to_string(i);

                    for (uint32 batchIdx = 0; batchIdx < entity.meshAsset->meshBatches.size(); ++batchIdx)
                    {
                        entity.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties
                        {
                            LinearColorConst::WHITE
                            , 1.0f, 1.0f
                            , textureScale
                            , textures[uint32(textures.size() * ud01(generator))]
                            , &texturedPipelineContext
                        });
                    }
                    pushEntity(entity);
                }

                // Near floor
                float height = 175;
                if (distribution1(generator) > 0.5f)
                {
                    SpotLight light;
                    light.radius = 700;
                    light.innerCone = 60;
                    light.outerCone = 80;
                    light.lumen = 200;
                    light.transform.setTranslation(offset + Vector3D(0, 0, height));

                    Vector3D dir = Vector3D(distribution(generator), distribution(generator), -0.5);
                    light.name = "spot0_" + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    light.transform.setRotation(RotationMatrix::fromX(dir).asRotation());
                    pushSpt(light);

                    dir = dir * Vector3D(-1, -1, 1);
                    light.name = "spot1_" + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    light.transform.setRotation(RotationMatrix::fromX(dir).asRotation());
                    pushSpt(light);
                }
                else
                {
                    PointLight light;
                    light.radius = 800;
                    light.lumen = 250;

                    light.lightPos = offset + Vector3D(400, 400, 130);
                    light.name = "point0_" + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);

                    light.lightPos = offset + Vector3D(400, -400, 130);
                    light.name = "point1_" + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);

                    light.lightPos = offset + Vector3D(-400, 400, 130);
                    light.name = "point2_" + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);

                    light.lightPos = offset + Vector3D(-400, -400, 130);
                    light.name = "point3_" + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);
                }


                // Pillars
                for (auto& batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.uvScale = pillarTextureScale;
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.meshAsset = cylinder;
                sceneFloor.transform.setScale(Vector3D(1, 1, 5));
                sceneFloor.transform.setTranslation(offset + Vector3D(450, 450, 250));
                sceneFloor.name = "pillar1_" + roomIdx;
                pushEntity(sceneFloor);

                for (auto& batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(-450, 450, 250));
                sceneFloor.name = "pillar2_" + roomIdx;
                pushEntity(sceneFloor);

                for (auto& batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(450, -450, 250));
                sceneFloor.name = "pillar3_" + roomIdx;
                pushEntity(sceneFloor);

                for (auto& batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(-450, -450, 250));
                sceneFloor.name = "pillar4_" + roomIdx;
                pushEntity(sceneFloor);
            }
        }
    }
    // Special scene
    {
        PBRSceneEntity carsFloor;
        carsFloor.name = "ShowroomFloor";
        carsFloor.meshAsset = cylinder;
        carsFloor.transform.setScale(Vector3D(13, 13, 1));
        carsFloor.transform.setTranslation(Vector3D(0, 2800, -50));
        for (uint32 batchIdx = 0; batchIdx < carsFloor.meshAsset->meshBatches.size(); ++batchIdx)
        {
            carsFloor.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties
            {
                LinearColorConst::WHITE
                , 1.f, 1.f
                , floorTextureScale
                , "Tiles074"
                , &texturedPipelineContext
            });
        }
        pushEntity(carsFloor);

        PBRSceneEntity car;
        car.name = "DodgeChallenger";
        car.meshAsset = static_cast<StaticMeshAsset*>(appInstance().assetManager.getAsset(car.name));
        fatalAssert(car.meshAsset, "%s() : Failed finding car mesh %s", __func__, car.name.getChar());
        car.transform.setTranslation(Vector3D(0, 2800, 0));
        for (uint32 batchIdx = 0; batchIdx < car.meshAsset->meshBatches.size(); ++batchIdx)
        {
            car.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties
            {
                LinearColorConst::WHITE
                , 1.f, 1.f
                , Vector2D::ONE
                , car.name + car.meshAsset->meshBatches[batchIdx].name
                , &texturedPipelineContext
            });
        }
        pushEntity(car);

        SpotLight heroLight;
        heroLight.name = "HeroLight";
        heroLight.transform.setTranslation(car.transform.getTranslation() + Vector3D(0, 0, 400));
        heroLight.transform.setRotation(Rotation(0, 90, 0));
        heroLight.radius = 600;
        heroLight.innerCone = 72;
        heroLight.outerCone = 76;
        heroLight.lightcolor = LinearColorConst::WHITE;
        heroLight.lumen = 500;
        pushSpt(heroLight);
    }

    sceneVolume.reinitialize(entities, Vector3D(50, 50, 50));
}

void ExperimentalEnginePBR::destroyScene()
{
    sceneData.clear();
}

void ExperimentalEnginePBR::createShaderParameters()
{
    IGraphicsInstance* graphicsInstance = getRenderManager()->getGraphicsInstance();
    const PipelineBase* singleColPipeline = static_cast<const GraphicsPipelineBase*>(singleColorPipelineContext.getPipeline());
    // Since view data and other view related data are at set 0
    viewParameters = GraphicsHelper::createShaderParameters(graphicsInstance, singleColPipeline->getParamLayoutAtSet(0));
    viewParameters->setResourceName("View");
    for (PBRSceneEntity& entity : sceneData)
    {
        entity.instanceParameters = GraphicsHelper::createShaderParameters(graphicsInstance, singleColPipeline->getParamLayoutAtSet(1));
        entity.instanceParameters->setResourceName(entity.name);
        entity.meshBatchParameters.resize(entity.meshAsset->meshBatches.size());
        uint32 meshBatchIdx = 0;
        for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
        {
            const PipelineBase* pipeline = static_cast<const GraphicsPipelineBase*>(entity.meshBatchProps[meshBatchIdx].pipeline->getPipeline());
            meshBatchParam = (GraphicsHelper::createShaderParameters(graphicsInstance, pipeline->getParamLayoutAtSet(2)));
            meshBatchParam->setResourceName(entity.name + "_MeshBatch_" + std::to_string(meshBatchIdx));
            ++meshBatchIdx;
        }
    }

    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    lightTextures.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadTextureDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadNormalDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadAmbientDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadRoughDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadMetalDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawQuadDepthDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));
    drawLitColorsDescs.setNewSwapchain(appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow()));

    // Light related descriptors
    // as 1 and 2 are textures and light data
    const GraphicsResource* pbrModelDescLayout = drawPbrPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    lightCommon = GraphicsHelper::createShaderParameters(graphicsInstance, pbrModelDescLayout, { 1,2 });
    lightCommon->setResourceName("LightCommon");
    uint32 lightDataCount = uint32(Math::max(1u, Math::max(scenePointLights.size(), sceneSpotLights.size())));
    lightDataCount = uint32(Math::ceil(lightDataCount / float(ARRAY_LENGTH(PBRLightArray::spotLits))));
    lightData.resize(lightDataCount);
    for (uint32 i = 0; i < lightDataCount; ++i)
    {
        // as 0 and 1 are light common and textures
        lightData[i] = GraphicsHelper::createShaderParameters(graphicsInstance, pbrModelDescLayout, { 0, 1 });
        lightData[i]->setResourceName("Light_" + std::to_string(i * ARRAY_LENGTH(PBRLightArray::spotLits)) + "to"
            + std::to_string(i * ARRAY_LENGTH(PBRLightArray::spotLits) + ARRAY_LENGTH(PBRLightArray::spotLits)));
    }

    const GraphicsResource* drawQuadDescLayout = resolveToPresentPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        const String iString = std::to_string(i);
        lightTextures.set(GraphicsHelper::createShaderParameters(graphicsInstance, pbrModelDescLayout, { 0,2 }), i);
        lightTextures.getResources()[i]->setResourceName("LightFrameCommon_" + iString);

        drawQuadTextureDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadTextureDescs.getResources()[i]->setResourceName("QuadUnlit_" + iString);
        drawQuadNormalDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadNormalDescs.getResources()[i]->setResourceName("QuadNormal_" + iString);
        drawQuadDepthDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadDepthDescs.getResources()[i]->setResourceName("QuadDepth_" + iString);
        drawQuadAmbientDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadAmbientDescs.getResources()[i]->setResourceName("QuadAmb_" + iString);
        drawQuadRoughDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadRoughDescs.getResources()[i]->setResourceName("QuadRough_" + iString);
        drawQuadMetalDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadMetalDescs.getResources()[i]->setResourceName("QuadMetal_" + iString);

        drawLitColorsDescs.set(GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawLitColorsDescs.getResources()[i]->setResourceName("QuadLit_" + iString);
    }

    clearInfoParams = GraphicsHelper::createShaderParameters(graphicsInstance, clearQuadPipelineContext.getPipeline()->getParamLayoutAtSet(0));
    clearInfoParams->setResourceName("ClearInfo");

    camViewAndInstanceParams = GraphicsHelper::createShaderParameters(graphicsInstance, drawLinesDWritePipelineCntxt.getPipeline()->getParamLayoutAtSet(0));
    camViewAndInstanceParams->setResourceName("CameraGizmo");

    camRTParams = GraphicsHelper::createShaderParameters(graphicsInstance, drawQuadDescLayout);
    camRTParams->setResourceName("CameraGizmoToScreenQuad");
}

void PointLight::update() const
{
    PbrPointLight ptLit;
    ptLit.ptLightColor_lumen = lightcolor;
    ptLit.ptLightColor_lumen.w() = lumen;
    ptLit.ptPos_radius = Vector4D(lightPos.x()
        , lightPos.y(), lightPos.z(), radius);
    paramCollection->setBuffer("ptLits", ptLit, index);
}

void SpotLight::update() const
{
    PbrSpotLight spotLit;
    Vector3D temp = transform.getRotation().fwdVector();
    spotLit.sptDirection = Vector4D(temp.x(), temp.y(), temp.z(), lumen);
    spotLit.sptPos_radius = Vector4D(transform.getTranslation().x(), transform.getTranslation().y(), transform.getTranslation().z(), radius);
    spotLit.sptLightColor_lumen = lightcolor;
    spotLit.sptLightColor_lumen.w() = lumen;
    spotLit.sptCone = Vector2D(Math::cos(Math::deg2Rad(innerCone * 0.5f)), Math::cos(Math::deg2Rad(outerCone * 0.5f)));
    paramCollection->setBuffer("spotLits", spotLit, index);
}

void DirectionalLight::update() const
{
    auto dirLit = PbrDirectionalLight{ Vector4D(lightcolor), direction.fwdVector() };
    dirLit.lightColor_lumen.w() = lumen;
    paramCollection->setBuffer("dirLit", dirLit);
}

void ExperimentalEnginePBR::setupShaderParameterParams()
{
    IGraphicsInstance* graphicsInstance = getRenderManager()->getGraphicsInstance();

    ViewData viewData;
    viewData.view = camera.viewMatrix();
    viewData.invView = viewData.view.inverse();
    viewData.projection = camera.projectionMatrix();
    viewData.invProjection = viewData.projection.inverse();
    viewParameters->setBuffer("viewData", viewData);
    viewParameters->init();
    for (PBRSceneEntity& entity : sceneData)
    {
        entity.instanceParameters->setMatrixParam("model", entity.transform.getTransformMatrix());
        entity.instanceParameters->setMatrixParam("invModel", entity.transform.getTransformMatrix().inverse());
        entity.instanceParameters->init();

        uint32 batchIdx = 0;
        for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
        {
            meshBatchParam->setVector4Param("meshColor", Vector4D(entity.meshBatchProps[batchIdx].color));
            meshBatchParam->setFloatParam("roughness", entity.meshBatchProps[batchIdx].roughness);
            meshBatchParam->setFloatParam("metallic", entity.meshBatchProps[batchIdx].metallic);
            meshBatchParam->setVector4Param("rm_uvScale"
                , Vector4D(entity.meshBatchProps[batchIdx].roughness
                    , entity.meshBatchProps[batchIdx].metallic
                    , entity.meshBatchProps[batchIdx].uvScale.x(), entity.meshBatchProps[batchIdx].uvScale.y()));

            meshBatchParam->setTextureParam("diffuseMap", static_cast<TextureAsset*>(
                appInstance().assetManager.getAsset(entity.meshBatchProps[batchIdx].textureName + "_D"))->getTexture()->getTextureResource(), linearFiltering);
            meshBatchParam->setTextureParam("normalMap", static_cast<TextureAsset*>(
                appInstance().assetManager.getAsset(entity.meshBatchProps[batchIdx].textureName + "_N"))->getTexture()->getTextureResource(), linearFiltering);
            meshBatchParam->setTextureParam("armMap", static_cast<TextureAsset*>(
                appInstance().assetManager.getAsset(entity.meshBatchProps[batchIdx].textureName + "_ARM"))->getTexture()->getTextureResource(), linearFiltering);

            meshBatchParam->init();
            ++batchIdx;
        }
    }

    lightCommon->setBuffer("viewData", viewData);
    lightCommon->init();
    uint32 lightStartIdx = 0;

    // Directional light at last to do Linear -> SRGB and ambient lights
    dirLight.paramCollection = lightData[lightData.size() - 1];
    dirLight.update();
    lightData[lightData.size() - 1]->setFloatParam("gamma", gamma);
    lightData[lightData.size() - 1]->setFloatParam("exposure", exposure);
    lightData[lightData.size() - 1]->setIntParam("count", 0x00000100u);
    for (SharedPtr<ShaderParameters>& light : lightData)
    {
        uint32 count = light->getUintParam("count");
        // clear 0-5 bits
        count &= ~0x000000FF;

        uint32 rangeIdx = 0;
        for (; rangeIdx < ARRAY_LENGTH(PBRLightArray::spotLits) && (rangeIdx + lightStartIdx) < sceneSpotLights.size(); ++rangeIdx)
        {
            SpotLight& lightData = sceneSpotLights[rangeIdx + lightStartIdx];
            lightData.paramCollection = light;
            lightData.index = rangeIdx;

            lightData.update();
        }
        count |= (0x0000000F & rangeIdx);
        rangeIdx = 0;
        for (; rangeIdx < ARRAY_LENGTH(PBRLightArray::ptLits) && (rangeIdx + lightStartIdx) < scenePointLights.size(); ++rangeIdx)
        {
            PointLight& lightData = scenePointLights[rangeIdx + lightStartIdx];
            lightData.paramCollection = light;
            lightData.index = rangeIdx;

            lightData.update();
        }
        count |= ((0x0000000F & rangeIdx) << 4);

        light->setIntParam("count", count);
        light->init();

        lightStartIdx += ARRAY_LENGTH(PBRLightArray::spotLits);
    }

    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    ImageViewInfo ambImageViewInfo;
    ambImageViewInfo.componentMapping.g = ambImageViewInfo.componentMapping.b
        = ambImageViewInfo.componentMapping.a = ambImageViewInfo.componentMapping.r = EPixelComponentMapping::R;
    ImageViewInfo roughImageViewInfo;
    roughImageViewInfo.componentMapping.g = roughImageViewInfo.componentMapping.b
        = roughImageViewInfo.componentMapping.a = roughImageViewInfo.componentMapping.r = EPixelComponentMapping::G;
    ImageViewInfo metalImageViewInfo;
    metalImageViewInfo.componentMapping.g = metalImageViewInfo.componentMapping.b
        = metalImageViewInfo.componentMapping.a = metalImageViewInfo.componentMapping.r = EPixelComponentMapping::B;
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
        lightTextures.getResources()[i]->setTextureParam("ssARM", multibuffer->textures[(2 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssDepth", multibuffer->textures[(3 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParamViewInfo("ssDepth", depthImageViewInfo);
        lightTextures.getResources()[i]->setTextureParam("ssColor", frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("brdfLUT", GlobalBuffers::integratedBrdfLUT()->getTextureResource(), nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("envMap", envMaps[selectedEnv]->getEnvironmentMap()->getTextureResource(), linearFiltering);
        lightTextures.getResources()[i]->setTextureParam("diffuseIrradMap", envMaps[selectedEnv]->getDiffuseIrradianceMap()->getTextureResource(), linearFiltering);
        lightTextures.getResources()[i]->setTextureParam("specEnvMap", envMaps[selectedEnv]->getSpecularIrradianceMap()->getTextureResource(), linearFiltering);

        drawQuadTextureDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadNormalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadAmbientDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(2 * fbIncrement)], linearFiltering);
        drawQuadRoughDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(2 * fbIncrement)], linearFiltering);
        drawQuadMetalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(2 * fbIncrement)], linearFiltering);
        drawQuadAmbientDescs.getResources()[i]->setTextureParamViewInfo("quadTexture", ambImageViewInfo);
        drawQuadRoughDescs.getResources()[i]->setTextureParamViewInfo("quadTexture", roughImageViewInfo);
        drawQuadMetalDescs.getResources()[i]->setTextureParamViewInfo("quadTexture", metalImageViewInfo);
        drawQuadDepthDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(3 * fbIncrement)], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParamViewInfo("quadTexture", depthImageViewInfo);

        drawLitColorsDescs.getResources()[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
    }
    lightTextures.init();
    drawQuadTextureDescs.init();
    drawQuadNormalDescs.init();
    drawQuadAmbientDescs.init();
    drawQuadRoughDescs.init();
    drawQuadMetalDescs.init();
    drawQuadDepthDescs.init();
    drawLitColorsDescs.init();

    clearInfoParams->setVector4Param("clearColor", Vector4D(0, 0, 0, 0));
    clearInfoParams->init();

    Camera gizmoCamera;
    gizmoCamera.setClippingPlane(5.f, 305.f);
    gizmoCamera.setOrthoSize({ 290, 290 });
    gizmoCamera.cameraProjection = ECameraProjection::Orthographic;
    updateCamGizmoViewParams();
    camViewAndInstanceParams->setMatrixParam("projection", gizmoCamera.projectionMatrix());
    camViewAndInstanceParams->setMatrixParam("model", Matrix4::IDENTITY);
    camViewAndInstanceParams->init();

    camRTParams->setTextureParam("quadTexture", camGizmoColorTexture->getTextureResource(), linearFiltering);
    camRTParams->init();
}

void ExperimentalEnginePBR::updateShaderParameters(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
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

void ExperimentalEnginePBR::reupdateTextureParamsOnResize()
{
    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* multibuffer = GlobalBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, i);
        const int32 fbIncrement = multibuffer->bHasResolves ? 2 : 1;
        const int32 resolveIdxOffset = multibuffer->bHasResolves ? 1 : 0;
        lightTextures.getResources()[i]->setTextureParam("ssUnlitColor", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssNormal", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssARM", multibuffer->textures[(2 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssDepth", multibuffer->textures[(3 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam("ssColor", frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering);

        drawQuadTextureDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(0 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadNormalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(1 * fbIncrement) + resolveIdxOffset], linearFiltering);
        drawQuadAmbientDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(2 * fbIncrement)], linearFiltering);
        drawQuadRoughDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(2 * fbIncrement)], linearFiltering);
        drawQuadMetalDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(2 * fbIncrement)], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParam("quadTexture", multibuffer->textures[(3 * fbIncrement)], linearFiltering);
        drawLitColorsDescs.getResources()[i]->setTextureParam("quadTexture", frameResources[i].lightingPassRt->getTextureResource(), linearFiltering);
    }
}

void ExperimentalEnginePBR::reupdateEnvMap()
{
    ENQUEUE_COMMAND(WaitEnvMapUpdate)(
        [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            cmdList->flushAllcommands();
            const uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
            for (uint32 i = 0; i < swapchainCount; ++i)
            {
                lightTextures.getResources()[i]->setTextureParam("envMap", envMaps[selectedEnv]->getEnvironmentMap()->getTextureResource(), linearFiltering);
                lightTextures.getResources()[i]->setTextureParam("diffuseIrradMap", envMaps[selectedEnv]->getDiffuseIrradianceMap()->getTextureResource(), linearFiltering);
                lightTextures.getResources()[i]->setTextureParam("specEnvMap", envMaps[selectedEnv]->getSpecularIrradianceMap()->getTextureResource(), linearFiltering);
            }
        }
    );
}

void ExperimentalEnginePBR::destroyShaderParameters()
{
    viewParameters->release();
    viewParameters.reset();

    for (PBRSceneEntity& entity : sceneData)
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
    drawQuadAmbientDescs.reset();
    drawQuadRoughDescs.reset();
    drawQuadMetalDescs.reset();
    drawQuadDepthDescs.reset();
    drawLitColorsDescs.reset();

    clearInfoParams->release();
    clearInfoParams.reset();

    camViewAndInstanceParams->release();
    camViewAndInstanceParams.reset();

    camRTParams->release();
    camRTParams.reset();
}

void ExperimentalEnginePBR::resizeLightingRts(const Size2D& size)
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        frameResources[i].lightingPassRt->setTextureSize(size);
        frameResources[i].lightingPassResolved->setTextureSize(size);
        getRenderManager()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt });
        getRenderManager()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassResolved });

        // Used in debug rendering using depth map as read only target
        getRenderManager()->getGlobalRenderingContext()->clearExternInitRtsFramebuffer({ frameResources[i].lightingPassRt, GlobalBuffers::getFramebufferRts(ERenderPassFormat::Multibuffers, i)[3] });
    }
}

void ExperimentalEnginePBR::createFrameResources()
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
        frameResources[i].recordingFence = GraphicsHelper::createFence(getRenderManager()->getGraphicsInstance(), (name + "RecordingGaurd").c_str(), true);

        rtCreateParams.textureName = "LightingRT_" + std::to_string(i);
        frameResources[i].lightingPassRt = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
        rtCreateParams.textureName = "LightingResolved_" + std::to_string(i);
        frameResources[i].lightingPassResolved = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
    }
}

void ExperimentalEnginePBR::destroyFrameResources()
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

void ExperimentalEnginePBR::getPipelineForSubpass()
{
    VulkanGlobalRenderingContext* vulkanRenderingContext = static_cast<VulkanGlobalRenderingContext*>(getRenderManager()->getGlobalRenderingContext());

    singleColorPipelineContext.forVertexType = EVertexType::StaticMesh;
    singleColorPipelineContext.materialName = "SingleColor";
    singleColorPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffers;
    singleColorPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&singleColorPipelineContext);

    texturedPipelineContext.forVertexType = EVertexType::StaticMesh;
    texturedPipelineContext.materialName = "Textured";
    texturedPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffers;
    texturedPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&texturedPipelineContext);

    // Gooch model
    drawPbrPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawPbrPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassRt);
    drawPbrPipelineContext.materialName = "PBR";
    vulkanRenderingContext->preparePipelineContext(&drawPbrPipelineContext);

    sceneDebugLinesPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    sceneDebugLinesPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassRt);
    // Using depth map as read only target
    sceneDebugLinesPipelineContext.rtTextures.emplace_back(GlobalBuffers::getFramebufferRts(ERenderPassFormat::Multibuffers, 0)[3]);
    sceneDebugLinesPipelineContext.materialName = "Draw3DColoredPerVertexLine";
    vulkanRenderingContext->preparePipelineContext(&sceneDebugLinesPipelineContext);

    drawLinesDWritePipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
    drawLinesDWritePipelineCntxt.rtTextures.emplace_back(camGizmoColorTexture);
    drawLinesDWritePipelineCntxt.rtTextures.emplace_back(camGizmoDepthTarget);
    drawLinesDWritePipelineCntxt.materialName = "Draw3DColoredPerVertexLineDWrite";
    vulkanRenderingContext->preparePipelineContext(&drawLinesDWritePipelineCntxt);

    clearQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    clearQuadPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassResolved);
    clearQuadPipelineContext.materialName = "ClearRT";
    vulkanRenderingContext->preparePipelineContext(&clearQuadPipelineContext);

    resolveLightRtPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    resolveLightRtPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassResolved);
    resolveLightRtPipelineContext.materialName = "DrawQuadFromTexture";
    vulkanRenderingContext->preparePipelineContext(&resolveLightRtPipelineContext);

    resolveToPresentPipelineContext.bUseSwapchainFb = true;
    resolveToPresentPipelineContext.materialName = "DrawQuadFromTexture";
    resolveToPresentPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    resolveToPresentPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&resolveToPresentPipelineContext);

    overBlendedQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    overBlendedQuadPipelineContext.rtTextures.emplace_back(frameResources[0].lightingPassRt);
    overBlendedQuadPipelineContext.materialName = "DrawOverBlendedQuadFromTexture";
    vulkanRenderingContext->preparePipelineContext(&overBlendedQuadPipelineContext);
}

void ExperimentalEnginePBR::createPipelineResources()
{
    clearValues.colors.resize(singleColorPipelineContext.getFb()->textures.size(), LinearColorConst::BLACK);

    // Shader pipeline's buffers and image access
    createShaderParameters();
}

void ExperimentalEnginePBR::destroyPipelineResources()
{
    // Shader pipeline's buffers and image access
    destroyShaderParameters();
}

void ExperimentalEnginePBR::updateCameraParams()
{
    ViewData viewDataTemp;
    bool bCamRotated = false;
    if (appInstance().inputSystem()->isKeyPressed(Keys::RMB))
    {
        cameraRotation.yaw() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseX)->currentValue * timeData.activeTimeDilation * 0.25f;
        cameraRotation.pitch() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseY)->currentValue * timeData.activeTimeDilation * 0.25f;
        bCamRotated = true;
    }
    float camSpeedModifier = 1;
    if (appInstance().inputSystem()->isKeyPressed(Keys::LSHIFT))
    {
        camSpeedModifier = 2;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::A))
    {
        cameraTranslation -= cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::D))
    {
        cameraTranslation += cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::W))
    {
        cameraTranslation += cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::S))
    {
        cameraTranslation -= cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::Q))
    {
        cameraTranslation -= Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::E))
    {
        cameraTranslation += Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (appInstance().inputSystem()->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = RotationMatrix::fromZX(Vector3D::UP, cameraRotation.fwdVector()).asRotation();
        bCamRotated = true;
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

    if (bCamRotated)
    {
        updateCamGizmoViewParams();
        ENQUEUE_COMMAND_NODEBUG(CameraGizmoUpdate,
            {
                updateCamGizmoCapture(cmdList, graphicsInstance);
            }, this);
    }
}

void ExperimentalEnginePBR::onStartUp()
{
    GameEngine::onStartUp();

    ENQUEUE_COMMAND_NODEBUG(EngineStartUp,
        {
            startUpRenderInit();
            updateCamGizmoCapture(cmdList, graphicsInstance);
        }, this);

    camera.cameraProjection = projection;
    camera.setOrthoSize({ 1280,720 });
    camera.setClippingPlane(0.1f, 6000.f);
    camera.setFOV(110.f, 90.f);

    cameraTranslation = Vector3D(0.f, 1.f, 0.0f).safeNormalize() * (500);
    cameraTranslation.z() += 200;

    camera.setTranslation(cameraTranslation);
    camera.lookAt(Vector3D::ZERO);
    cameraRotation = camera.rotation();

    getRenderManager()->getImGuiManager()->addLayer(this);
    createScene();

    textures = getApplicationInstance()->assetManager.getAssetsOfType<EAssetType::Texture2D, TextureAsset>();
    std::sort(textures.begin(), textures.end(), SortAssetByName<true>());
    textureNames.reserve(textures.size() + 1);
    textureNames.emplace_back(noneString.getChar());
    for (const TextureAsset* texture : textures)
    {
        textureNames.emplace_back(texture->assetName().getChar());
    }
    selectedTexture = 0;

    envMaps = getApplicationInstance()->assetManager.getAssetsOfType<EAssetType::CubeMap, EnvironmentMapAsset>();
    std::sort(envMaps.begin(), envMaps.end(), SortAssetByName<true>());
    envMapNames.reserve(envMaps.size());
    for (const EnvironmentMapAsset* envMap : envMaps)
    {
        envMapNames.emplace_back(envMap->assetName().getChar());
    }
    selectedEnv = 0;

    tempTest();
}

void ExperimentalEnginePBR::startUpRenderInit()
{
    vDevice = VulkanGraphicsHelper::getVulkanDevice(getRenderManager()->getGraphicsInstance());
    device = VulkanGraphicsHelper::getDevice(vDevice);
    graphicsDbg = VulkanGraphicsHelper::debugGraphics(getRenderManager()->getGraphicsInstance());
    createPools();
    frameResources.resize(getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount());

    createFrameResources();
    createImages();
    getPipelineForSubpass();
    createPipelineResources();
    setupShaderParameterParams();
}

void ExperimentalEnginePBR::onQuit()
{
    ENQUEUE_COMMAND_NODEBUG(EngineQuit, { renderQuit(); }, this);

    getRenderManager()->getImGuiManager()->removeLayer(this);
    GameEngine::onQuit();
}

void ExperimentalEnginePBR::renderQuit()
{
    vDevice->vkDeviceWaitIdle(device);

    destroyPipelineResources();
    destroyImages();
    destroyFrameResources();

    destroyScene();

    destroyPools();
}

void ExperimentalEnginePBR::frameRender(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    singleColorPipelineContext.swapchainIdx = resolveToPresentPipelineContext.swapchainIdx = index;
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&singleColorPipelineContext);
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&resolveToPresentPipelineContext);

    drawPbrPipelineContext.rtTextures[0] = frameResources[index].lightingPassRt;
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&drawPbrPipelineContext);
    resolveLightRtPipelineContext.rtTextures[0] = frameResources[index].lightingPassResolved;
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&resolveLightRtPipelineContext);

    std::map<LocalPipelineContext*, std::map<const PBRSceneEntity*, std::vector<uint32>>> drawingPipelineToEntities;
    for (const PBRSceneEntity& entity : sceneData)
    {
        uint32 meshBatchIdx = 0;
        for (const PBRSceneEntity::BatchProperties& meshBatchProp : entity.meshBatchProps)
        {
            drawingPipelineToEntities[meshBatchProp.pipeline][&entity].emplace_back(meshBatchIdx);
            ++meshBatchIdx;
        }
    }

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

    const GraphicsResource* cmdBuffer = cmdList->startCmd(cmdName, EQueueFunction::Graphics, true);
    VkCommandBuffer frameCmdBuffer = VulkanGraphicsHelper::getRawCmdBuffer(graphicsInstance, cmdBuffer);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ExperimentalEnginePBRFrame);

        cmdList->cmdBeginRenderPass(cmdBuffer, singleColorPipelineContext, scissor, {}, clearValues);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, MainUnlitPass);

            // View set
            cmdList->cmdBindDescriptorsSets(cmdBuffer, singleColorPipelineContext, viewParameters.get());
            for (const std::pair<const LocalPipelineContext*, std::map<const PBRSceneEntity*, std::vector<uint32>>>& pipelineEntities : drawingPipelineToEntities)
            {
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, *pipelineEntities.first, { queryParam });

                for (const std::pair<const PBRSceneEntity* const, std::vector<uint32>>& entity : pipelineEntities.second)
                {
                    // Instance set
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, *pipelineEntities.first, entity.first->instanceParameters.get());

                    cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { entity.first->meshAsset->vertexBuffer }, { 0 });
                    cmdList->cmdBindIndexBuffer(cmdBuffer, entity.first->meshAsset->indexBuffer);

                    for (const uint32& entityBatch : entity.second)
                    {
                        // Material set
                        cmdList->cmdBindDescriptorsSets(cmdBuffer, *pipelineEntities.first, entity.first->meshBatchParameters[entityBatch].get());
                        cmdList->cmdDrawIndexed(cmdBuffer
                            , entity.first->meshAsset->meshBatches[entityBatch].startIndex
                            , entity.first->meshAsset->meshBatches[entityBatch].numOfIndices);
                    }
                }
            }
        }
        cmdList->cmdEndRenderPass(cmdBuffer);

        // Drawing lighting quads
        viewport.minBound = Int2D(0, 0);
        viewport.maxBound = EngineSettings::screenSize.get();

        cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { GlobalBuffers::getQuadVertexIndexBuffers().first }, { 0 });
        cmdList->cmdBindIndexBuffer(cmdBuffer, GlobalBuffers::getQuadVertexIndexBuffers().second);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        if (frameVisualizeId == 0)
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
                cmdList->cmdBeginRenderPass(cmdBuffer, drawPbrPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawLight);
                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawPbrPipelineContext, { queryParam });

                    cmdList->cmdBindDescriptorsSets(cmdBuffer, drawPbrPipelineContext, { lightCommon.get(), *lightTextures, light.get() });
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
                drawQuadDescs = *drawQuadAmbientDescs;
                break;
            case 4:
                drawQuadDescs = *drawQuadRoughDescs;
                break;
            case 5:
                drawQuadDescs = *drawQuadMetalDescs;
                break;
            case 6:
                drawQuadDescs = *drawQuadDepthDescs;
                break;
            case 0:
            default:
                break;
            }

            if (drawQuadDescs)
            {
                resolveLightRtPipelineContext.rtTextures = drawPbrPipelineContext.rtTextures;
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

        // Debug Draw
        debugFrameRender(cmdList, graphicsInstance, cmdBuffer, index);

        // Drawing IMGUI
        TinyDrawingContext drawingContext;
        drawingContext.cmdBuffer = cmdBuffer;
        drawingContext.rtTextures = drawPbrPipelineContext.rtTextures;
        getRenderManager()->getImGuiManager()->draw(cmdList, graphicsInstance, drawingContext);

        // Drawing final quad
        viewport.minBound = Int2D(0, 0);
        viewport.maxBound = scissor.maxBound = EngineSettings::surfaceSize.get();

        cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { GlobalBuffers::getQuadVertexIndexBuffers().first }, { 0 });
        cmdList->cmdBindIndexBuffer(cmdBuffer, GlobalBuffers::getQuadVertexIndexBuffers().second);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        RenderPassAdditionalProps renderPassAdditionalProps;
        renderPassAdditionalProps.bUsedAsPresentSource = true;
        cmdList->cmdBeginRenderPass(cmdBuffer, resolveToPresentPipelineContext, scissor, renderPassAdditionalProps, clearValues);

        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveToSwapchain);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, resolveToPresentPipelineContext, { queryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, resolveToPresentPipelineContext, *drawLitColorsDescs);
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

void ExperimentalEnginePBR::updateCamGizmoViewParams()
{
    Camera gizmoCam;
    gizmoCam.setTranslation(-camera.rotation().fwdVector() * 150);
    gizmoCam.lookAt(Vector3D::ZERO);

    camViewAndInstanceParams->setMatrixParam("invView", gizmoCam.viewMatrix().inverse());
}

void ExperimentalEnginePBR::updateCamGizmoCapture(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    String cmdName = "UpdateCameraGizmo";
    cmdList->finishCmd(cmdName);

    const GraphicsResource* cmdBuffer = cmdList->getCmdBuffer(cmdName);
    if (cmdBuffer == nullptr)
    {
        GraphicsPipelineState pipelineState;
        pipelineState.pipelineQuery = { EPolygonDrawMode::Fill, ECullingMode::BackFace };
        pipelineState.lineWidth = 3.0f;

        QuantizedBox2D viewport;
        // Since view matrix positive y is along up while vulkan positive y in view is down
        viewport.minBound.x = 0;
        viewport.minBound.y = camGizmoColorTexture->getTextureSize().y;
        viewport.maxBound.x = camGizmoColorTexture->getTextureSize().x;
        viewport.maxBound.y = 0;

        QuantizedBox2D scissor;
        scissor.minBound = { 0, 0 };
        scissor.maxBound = camGizmoColorTexture->getTextureSize();

        RenderPassClearValue clearVal;
        clearVal.colors.emplace_back(Color());
        // Record once
        cmdBuffer = cmdList->startCmd(cmdName, EQueueFunction::Graphics, false);
        cmdList->cmdBeginRenderPass(cmdBuffer, drawLinesDWritePipelineCntxt, scissor, {}, clearVal);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, UpdateCameraGizmo);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawLinesDWritePipelineCntxt, pipelineState);

            cmdList->cmdPushConstants(cmdBuffer, sceneDebugLinesPipelineContext, { {"ptSize", 1.0f} });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawLinesDWritePipelineCntxt, camViewAndInstanceParams.get());
            cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { GlobalBuffers::getLineGizmoVertexIndexBuffers().first }, { 0 });
            cmdList->cmdBindIndexBuffer(cmdBuffer, GlobalBuffers::getLineGizmoVertexIndexBuffers().second);

            cmdList->cmdDrawIndexed(cmdBuffer, 0, GlobalBuffers::getLineGizmoVertexIndexBuffers().second->bufferCount());
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
        cmdList->endCmd(cmdBuffer);
    }

    CommandSubmitInfo2 cmdSubmit;
    cmdSubmit.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, cmdSubmit);
}

void ExperimentalEnginePBR::debugFrameRender(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsResource* cmdBuffer, uint32 swapchainIdx)
{
    GraphicsPipelineQueryParams backfaceFillQueryParam;
    backfaceFillQueryParam.cullingMode = ECullingMode::BackFace;
    backfaceFillQueryParam.drawMode = EPolygonDrawMode::Fill;

    // Drawing in scene first
    QuantizedBox2D viewport;
    // Since view matrix positive y is along up while vulkan positive y in view is down
    viewport.minBound.x = 0;
    viewport.minBound.y = EngineSettings::screenSize.get().y;
    viewport.maxBound.x = EngineSettings::screenSize.get().x;
    viewport.maxBound.y = 0;

    QuantizedBox2D scissor;
    scissor.minBound = { 0, 0 };
    scissor.maxBound = EngineSettings::screenSize.get();

#if _DEBUG
    sceneDebugLinesPipelineContext.rtTextures[0] = frameResources[swapchainIdx].lightingPassRt;
    sceneDebugLinesPipelineContext.rtTextures[1] = GlobalBuffers::getFramebufferRts(ERenderPassFormat::Multibuffers, swapchainIdx)[3];
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&sceneDebugLinesPipelineContext);

    if (bDrawTbn && selection.type == GridEntity::Entity)
    {
        PBRSceneEntity& sceneEntity = sceneData[selection.idx];
        // Resetting viewport as we use mvp again
        viewport.minBound.x = 0;
        viewport.minBound.y = EngineSettings::screenSize.get().y;
        viewport.maxBound.x = EngineSettings::screenSize.get().x;
        viewport.maxBound.y = 0;

        SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawTBN);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        RenderPassAdditionalProps debugSceneDrawAdditionalProps;
        debugSceneDrawAdditionalProps.depthLoadOp = EAttachmentOp::LoadOp::Load;
        debugSceneDrawAdditionalProps.depthStoreOp = EAttachmentOp::StoreOp::DontCare;
        debugSceneDrawAdditionalProps.colorAttachmentLoadOp = EAttachmentOp::LoadOp::Load;
        cmdList->cmdBeginRenderPass(cmdBuffer, sceneDebugLinesPipelineContext, scissor, debugSceneDrawAdditionalProps, clearValues);
        {
            GraphicsPipelineState pipelineState;
            pipelineState.pipelineQuery = backfaceFillQueryParam;
            pipelineState.lineWidth = 1.0f;
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, sceneDebugLinesPipelineContext, pipelineState);
            cmdList->cmdBindDescriptorsSets(cmdBuffer, sceneDebugLinesPipelineContext, { viewParameters.get(), sceneEntity.instanceParameters.get() });
            cmdList->cmdPushConstants(cmdBuffer, sceneDebugLinesPipelineContext, { {"ptSize", 1.0f} });
            cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { sceneEntity.meshAsset->getTbnVertexBuffer() }, { 0 });

            cmdList->cmdDrawVertices(cmdBuffer, 0, uint32(sceneEntity.meshAsset->tbnVerts.size()));
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
#endif
    overBlendedQuadPipelineContext.rtTextures[0] = frameResources[swapchainIdx].lightingPassRt;
    getRenderManager()->getGlobalRenderingContext()->preparePipelineContext(&overBlendedQuadPipelineContext);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawCameraGizmoRT);

        RenderPassAdditionalProps drawOverlay;
        drawOverlay.colorAttachmentLoadOp = EAttachmentOp::LoadOp::Load;

        const Int2D margin(10, 10);

        Vector2D viewportSize = (Vector2D(camGizmoColorTexture->getTextureSize()) / Vector2D(3840, 2160)) * Vector2D(EngineSettings::screenSize.get());
        viewport.minBound = Int2D(0 + margin.x, EngineSettings::screenSize.get().y - int32(viewportSize.y()) - margin.y);
        viewport.maxBound = viewport.minBound + Int2D(viewportSize.x(), viewportSize.y());

        scissor = viewport;

        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBeginRenderPass(cmdBuffer, overBlendedQuadPipelineContext, viewport, drawOverlay, clearValues);
        {
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, overBlendedQuadPipelineContext, { backfaceFillQueryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, overBlendedQuadPipelineContext, { camRTParams.get() });
            cmdList->cmdBindVertexBuffers(cmdBuffer, 0, { GlobalBuffers::getQuadVertexIndexBuffers().first }, { 0 });
            cmdList->cmdBindIndexBuffer(cmdBuffer, GlobalBuffers::getQuadVertexIndexBuffers().second);

            cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
}

void ExperimentalEnginePBR::tickEngine()
{
    GameEngine::tickEngine();
    updateCameraParams();

    if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::ONE))
    {
        frameVisualizeId = 0;
    }
    else if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::TWO))
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

    if (getApplicationInstance()->inputSystem()->keyState(Keys::LMB)->keyWentDown && !getRenderManager()->getImGuiManager()->capturedInputs())
    {
        Rect windowArea = getApplicationInstance()->appWindowManager.getMainWindow()->windowClientRect();
        Vector2D mouseCoord = Vector2D(getApplicationInstance()->inputSystem()->analogState(AnalogStates::AbsMouseX)->currentValue
            , getApplicationInstance()->inputSystem()->analogState(AnalogStates::AbsMouseY)->currentValue) - windowArea.minBound;
        mouseCoord /= Vector2D(EngineSettings::surfaceSize.get());
        Logger::debug("ExperimentalEnginePBR", "%s(): mouse coord (%f, %f)", __func__, mouseCoord.x(), mouseCoord.y());
        if (mouseCoord.x() >= 0 && mouseCoord.x() <= 1.0f && mouseCoord.y() >= 0 && mouseCoord.y() <= 1.0f)
        {
            Vector3D worldFwd = camera.screenToWorldFwd(mouseCoord);
            std::vector<GridEntity> entities;
            if (sceneVolume.raycast(camera.translation(), worldFwd, 2000, entities))
            {
                selection = entities.front();
            }
            else
            {
                selection.type = GridEntity::Invalid;
            }
        }
    }

    if (renderSize != EngineSettings::screenSize.get())
    {
        ENQUEUE_COMMAND(WritingDescs)(
            [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
            {
                GlobalBuffers::onScreenResized(renderSize);
                resizeLightingRts(renderSize);
                reupdateTextureParamsOnResize();
                EngineSettings::screenSize.set(renderSize);
            });
    }

    ENQUEUE_COMMAND_NODEBUG(TickFrame,
        {
            updateShaderParameters(cmdList, graphicsInstance);
            frameRender(cmdList, graphicsInstance);
        }, this);

    tempTestPerFrame();
}

int32 ExperimentalEnginePBR::layerDepth() const
{
    return 0;
}

int32 ExperimentalEnginePBR::sublayerDepth() const
{
    return 0;
}

void ExperimentalEnginePBR::draw(class ImGuiDrawInterface* drawInterface)
{
    static bool bOpenImguiDemo = false;
    static bool bOpenImPlotDemo = false;
    if (bOpenImguiDemo)
    {
        ImGui::ShowDemoWindow(&bOpenImguiDemo);
    }
    if (bOpenImPlotDemo)
    {
        ImPlot::ShowDemoWindow(&bOpenImPlotDemo);
    }

    static bool bSettingOpen = true;

    if (bSettingOpen)
    {
        ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Settings", &bSettingOpen, ImGuiWindowFlags_NoMove))
        {
            ImGui::End();
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

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
                    const char* bufferMode[] = { "Lit", "Unlit", "Normal","AO", "Roughness", "metallic", "Depth" };
                    ImGui::Combo("Frame", &frameVisualizeId, bufferMode, ARRAY_LENGTH(bufferMode));
                }

                ImGui::Separator();
                ImGui::NextColumn();
                ImGui::Text("Exposure");
                ImGui::NextColumn();
                if (ImGui::InputFloat("Exp", &exposure, 1.0f, 10.f, "%.1f"))
                {
                    dirLight.paramCollection->setFloatParam("exposure", exposure);
                }

                ImGui::NextColumn();
                ImGui::Text("Gamma");
                ImGui::NextColumn();
                if (ImGui::InputFloat("Gamma", &gamma, 1.0f, 4.f, "%.1f"))
                {
                    dirLight.paramCollection->setFloatParam("gamma", gamma);
                }

                ImGui::NextColumn();
                ImGui::Text("Env Map");
                ImGui::NextColumn();
                if (ImGui::Combo("maps", &selectedEnv, envMapNames.data(), int32(envMapNames.size())))
                {
                    reupdateEnvMap();
                }
#if _DEBUG
                ImGui::NextColumn();
                ImGui::Text("Draw TBN");
                ImGui::NextColumn();
                ImGui::Checkbox("TBN Debug", &bDrawTbn);
#endif
            };

            ImGui::Columns(1);
            ImGui::NextColumn();
            if (ImGui::CollapsingHeader("Properties"))
            {
                ImGui::TreePush("SelectionNode");
                if (selection.type != GridEntity::Invalid)
                    drawSelectionWidget(drawInterface);
                ImGui::TreePop();

                ImGui::Separator();
                ImGui::TreePush("DirectionalLightNode");
                if (ImGui::CollapsingHeader("Directional Light"))
                {
                    if (ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&dirLight.direction), 0.5f, -180.f, 180.f, "%.4f"))
                    {
                        dirLight.update();
                    }

                    if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&dirLight.lightcolor)))
                    {
                        Vector4D param{ dirLight.lightcolor };
                        param.w() = dirLight.lumen;
                        dirLight.paramCollection->setVector4Param("lightColor_lumen", param);
                    }

                    if (ImGui::InputFloat("Lumen", &dirLight.lumen, 1.0f, 10.f, "%.1f"))
                    {
                        Vector4D param{ dirLight.lightcolor };
                        param.w() = dirLight.lumen;
                        dirLight.paramCollection->setVector4Param("lightColor_lumen", param);
                    }
                }
                ImGui::TreePop();
            }

            //if (ImGui::CollapsingHeader("Textures"))
            //{
            //    ImGui::Image(static_cast<TextureAsset*>(appInstance().assetManager.getAsset("ten_N"))->getTexture(), ImVec2(
            //        ImGui::GetWindowContentRegionWidth(),
            //        ImGui::GetWindowContentRegionWidth()));
            //}
            if (ImGui::CollapsingHeader("Texture Histogram"))
            {
                if (selectedTexture != 0)
                {
                    ImGui::Image(textures[selectedTexture - 1]->getTexture(), ImVec2(64, 64));
                    ImGui::SameLine();
                }
                if (ImGui::Combo("Textures", &selectedTexture, textureNames.data(), int32(textureNames.size())))
                {
                    if (selectedTexture != 0)
                    {
                        ImageUtils::calcHistogramRGB(histogram[0].data(), histogram[1].data(), histogram[2].data(), 32
                            , reinterpret_cast<const uint8*>(textures[selectedTexture - 1]->getPixelData().data())
                            , textures[selectedTexture - 1]->getTexture()->getTextureSize().x
                            , textures[selectedTexture - 1]->getTexture()->getTextureSize().y, 4);
                    }
                }

                if (selectedTexture != 0)
                {
                    ImPlot::SetNextPlotLimits(0, 255, 0, 1.0, ImGuiCond_::ImGuiCond_Once);
                    if (ImPlot::BeginPlot("Texture Histogram", 0, 0, ImVec2(-1, 0), 0, ImPlotAxisFlags_::ImPlotAxisFlags_Lock, ImPlotAxisFlags_::ImPlotAxisFlags_Lock))
                    {
                        ImPlot::SetNextFillStyle(LinearColorConst::RED, 1.0f);
                        ImPlot::PlotShaded("Red", histogram[0].data(), int32(histogram[0].size()), 0.0f, 8);// 256/ binCount(32)

                        ImPlot::SetNextFillStyle(LinearColorConst::GREEN, 0.5f);
                        ImPlot::PlotShaded("Green", histogram[1].data(), int32(histogram[1].size()), 0.0f, 8);

                        ImPlot::SetNextFillStyle(LinearColorConst::BLUE, 0.5f);
                        ImPlot::PlotShaded("Blue", histogram[2].data(), int32(histogram[2].size()), 0.0f, 8);
                        ImPlot::EndPlot();
                    }
                }
            }
            ImGui::PopStyleVar();
            ImGui::End();
        }
    }

    // FPS
    ImGui::SetNextWindowSize(ImVec2(145, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 145, 0), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, LinearColor(0, 0, 0, 0.6f));
    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("%.3f ms(%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
    ImGui::PopStyleColor();
}

void ExperimentalEnginePBR::drawSelectionWidget(class ImGuiDrawInterface* drawInterface)
{
    if (ImGui::CollapsingHeader("Selection"))
    {
        switch (selection.type)
        {
        case GridEntity::Entity:
        {
            PBRSceneEntity& entity = sceneData[selection.idx];
            String name = entity.name;
            ImGui::Text("Selected Entity : %s", name.getChar());

            if (ImGui::CollapsingHeader("Transform"))
            {
                AABB currentBound = getBounds(selection);
                bool bTransformChanged = ImGui::DragFloat3("Translation", reinterpret_cast<float*>(&entity.transform.getTranslation()), 1.f);
                bTransformChanged = ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&entity.transform.getRotation()), 1.f, 0.0f, 360.0f) || bTransformChanged;
                bTransformChanged = ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&entity.transform.getScale()), 0.05f) || bTransformChanged;

                if (bTransformChanged)
                {
                    entity.instanceParameters->setMatrixParam("model", entity.transform.getTransformMatrix());
                    entity.instanceParameters->setMatrixParam("invModel", entity.transform.getTransformMatrix().inverse());

                    sceneVolume.updateBounds(selection, currentBound, getBounds(selection));
                }
            }

            if (ImGui::TreeNode("Materials"))
            {
                uint32 i = 0;
                for (SharedPtr<ShaderParameters>& meshBatchParam : entity.meshBatchParameters)
                {
                    String materialName = entity.meshAsset->meshBatches[i].name.empty()
                        ? ("Material " + std::to_string(i)) : entity.meshAsset->meshBatches[i].name;
                    if (ImGui::TreeNode(materialName.getChar()))
                    {
                        PBRSceneEntity::BatchProperties& props = entity.meshBatchProps[i];
                        if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&props.color)))
                        {
                            meshBatchParam->setVector4Param("meshColor", props.color);
                        }
                        if (ImGui::DragFloat("Roughness", &props.roughness, 0.05f, 0.0f, 1.0f))
                        {
                            meshBatchParam->setFloatParam("roughness", props.roughness);
                            meshBatchParam->setVector4Param("rm_uvScale", Vector4D(props.roughness, props.metallic, props.uvScale.x(), props.uvScale.y()));
                        }
                        if (ImGui::DragFloat("Metallic", &props.metallic, 0.05f, 0.0f, 1.0f))
                        {
                            meshBatchParam->setFloatParam("metallic", props.metallic);
                            meshBatchParam->setVector4Param("rm_uvScale", Vector4D(props.roughness, props.metallic, props.uvScale.x(), props.uvScale.y()));
                        }
                        if (ImGui::DragFloat2("UV scaling", reinterpret_cast<float*>(&props.uvScale), 0.5f, 0.01f))
                        {
                            meshBatchParam->setVector4Param("rm_uvScale", Vector4D(props.roughness, props.metallic, props.uvScale.x(), props.uvScale.y()));
                        }
                        ImGui::TreePop();
                    }
                    ++i;
                }
                ImGui::TreePop();
            }

            break;
        }
        case GridEntity::SpotLight:
        {
            SpotLight& entity = sceneSpotLights[selection.idx];
            String name = entity.name;
            ImGui::Text("Selected Spot light : %s", name.getChar());

            bool bNeedsUpdate = false;
            if (ImGui::CollapsingHeader("Transform"))
            {
                AABB currentBound = getBounds(selection);
                bool bTransformChanged = ImGui::DragFloat3("Translation", reinterpret_cast<float*>(&entity.transform.getTranslation()), 1.0f);
                bTransformChanged = ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&entity.transform.getRotation()), 1.0f, 0.0f, 360.0f) || bTransformChanged;

                if (bTransformChanged)
                {
                    sceneVolume.updateBoundsChecked(selection, currentBound, getBounds(selection));
                }
                bNeedsUpdate = bTransformChanged;
            }

            if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&entity.lightcolor)))
            {
                bNeedsUpdate = true;
            }
            if (ImGui::InputFloat("Lumen", &entity.lumen, 1.0f, 10.f))
            {
                bNeedsUpdate = true;
            }
            if (ImGui::InputFloat("Radius", &entity.radius, 1.0f, 10.f))
            {
                bNeedsUpdate = true;
            }
            if (ImGui::DragFloat("Inner Cone", &entity.innerCone, 0.5f, 0.0f, entity.outerCone))
            {
                bNeedsUpdate = true;
            }
            if (ImGui::DragFloat("Outer Cone", &entity.outerCone, 0.5f, entity.innerCone, 179.0f))
            {
                bNeedsUpdate = true;
            }

            if (bNeedsUpdate)
            {
                entity.update();
            }
            break;
        }
        case GridEntity::PointLight:
        {
            PointLight& entity = scenePointLights[selection.idx];
            String name = entity.name;
            ImGui::Text("Selected Point light : %s", name.getChar());

            bool bNeedsUpdate = false;

            AABB currentBound = getBounds(selection);
            if (ImGui::DragFloat3("Translation", reinterpret_cast<float*>(&entity.lightPos), 1.0f))
            {
                sceneVolume.updateBoundsChecked(selection, currentBound, getBounds(selection));
                bNeedsUpdate = true;
            }

            if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&entity.lightcolor)))
            {
                bNeedsUpdate = true;
            }
            if (ImGui::InputFloat("Lumen", &entity.lumen, 1.0f, 10.f))
            {
                bNeedsUpdate = true;
            }
            if (ImGui::InputFloat("Radius", &entity.radius, 1.0f, 10.f))
            {
                bNeedsUpdate = true;
            }

            if (bNeedsUpdate)
            {
                entity.update();
            }
            break;
        }
        case GridEntity::Invalid:
        default:
            return;
        }
    }
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    static ExperimentalEnginePBR gameEngine;
    return &gameEngine;
}
#endif