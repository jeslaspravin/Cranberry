/*!
 * \file ExperimentalEnginePBR.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Engine/TestGameEngine.h"
#if EXPERIMENTAL

#include "Assets/Asset/EnvironmentMapAsset.h"
#include "Assets/Asset/StaticMeshAsset.h"
#include "Assets/Asset/TextureAsset.h"
#include "Core/GBuffers.h"
#include "Core/Types/Textures/ImageUtils.h"
#include "Core/Types/Textures/RenderTargetTextures.h"
#include "Core/Types/Textures/Texture2D.h"
#include "Core/Types/Textures/TexturesBase.h"
#include "CoreObjectGC.h"
#include "GenericAppWindow.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"
#include "Widgets/ImGui/IImGuiLayer.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/ImGui/ImGuiLib/implot.h"
#include "IApplicationModule.h"
#include "ICoreObjectsModule.h"
#include "Modules/ModuleManager.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keys.h"
#include "Math/BVH.h"
#include "Math/CoreMathTypes.h"
#include "Math/Math.h"
#include "Math/MathEasing.h"
#include "Math/MathGeom.h"
#include "Math/Quaternion.h"
#include "Math/RotationMatrix.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Material/MaterialCommonUniforms.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/Scene/RenderScene.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Resources/BufferedResources.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/QueueResource.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "RenderApi/Shaders/EngineShaders/PBRShaders.h"
#include "RenderApi/Shaders/EngineShaders/ShadowDepthDraw.h"
#include "RenderApi/Shaders/EngineShaders/SingleColorShader.h"
#include "RenderApi/Shaders/EngineShaders/TexturedShader.h"
#include "String/StringID.h"
#include "Types/Camera/Camera.h"
#include "Types/Colors.h"
#include "Types/CompilerDefines.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Transform3D.h"
#include "Types/TypesInfo.h"
#include "WindowManager.h"
#include "ApplicationSettings.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/Threading/CoPaT/DispatchHelpers.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
#include "Types/Platform/Threading/CoPaT/CoroutineAwaitAll.h"

#include <array>
#include <map>
#include <random>
#include <unordered_set>
#include <memory_resource>

#include "Widgets/WidgetDrawContext.h"
#include "Widgets/WidgetWindow.h"

struct PBRSceneEntity
{
    struct BatchProperties
    {
        LinearColor color;
        float roughness;
        float metallic;
        Vector2D uvScale{ Vector2D::ONE };
        String textureName;
        LocalPipelineContext *pipeline;
    };
    Transform3D transform;
    class StaticMeshAsset *meshAsset;
    String name;
    std::vector<BatchProperties> meshBatchProps;

    // Generated
    // Per mesh batch instance and shader param index
    // since material index is within the instance data
    std::vector<uint32> instanceParamIdx;
    std::vector<uint32> batchShaderParamIdx;
    void updateInstanceParams(ShaderParametersRef &shaderParams, uint32 batchIdx);
    void updateInstanceParams(ShaderParametersRef &shaderParams)
    {
        for (uint32 i = 0; i < meshBatchProps.size(); ++i)
        {
            updateInstanceParams(shaderParams, i);
        }
    }
    void updateMaterialParams(
        ShaderParametersRef &shaderParams, const std::unordered_map<ImageResourceRef, uint32> &tex2dToBindlessIdx, uint32 batchIdx
    ) const;
};

struct FrameResource
{
    std::vector<SemaphoreRef> usageWaitSemaphore;
    RenderTargetTexture *lightingPassRt;
    RenderTargetTexture *lightingPassResolved;
    FenceRef recordingFence;
};

#define SHADOWS_USE_CULLED_DRAW_CMDS 1
struct PointLight
{
    Vector3D lightPos;
    LinearColor lightcolor;
    float radius;
    float lumen;
    String name;
    std::array<Camera, 6> views;

    ShaderParametersRef *paramCollection;
    ShaderParametersRef *shadowViewParams;
    RenderTargetTexture *shadowMap;
    BufferResourceRef *drawCmdsBuffer;
    uint32 drawCmdCount;
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
    Camera view;

    ShaderParametersRef *paramCollection;
    ShaderParametersRef *shadowViewParams;
    RenderTargetTexture *shadowMap;
    BufferResourceRef *drawCmdsBuffer;
    uint32 drawCmdCount;
    uint32 index; // Index in param collection

    void update() const;
};

struct CascadeData
{
    Camera cascadeView;
    float frustumFarDistance;
    float frustumFract = 1.f;
};

struct DirectionalLight
{
    Rotation direction;
    LinearColor lightcolor;
    float lumen;

    uint32 cascadeCount = 4;
    std::vector<CascadeData> cascades;

    ShaderParametersRef *paramCollection;
    ShaderParametersRef *shadowViewParams;
    RenderTargetTexture *cascadeShadowMaps;

    void update() const;
    void normalizeCascadeCoverage();
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
};

FORCE_INLINE bool operator==(const GridEntity &lhs, const GridEntity &rhs) { return lhs.type == rhs.type && lhs.idx == rhs.idx; }

FORCE_INLINE bool operator<(const GridEntity &lhs, const GridEntity &rhs)
{
    return lhs.type < rhs.type || (lhs.type == rhs.type && lhs.idx < rhs.idx);
}

template <>
struct std::hash<GridEntity>
{
    NODISCARD size_t operator()(const GridEntity &keyval) const noexcept
    {
        size_t outHash = HashUtility::hash(keyval.type);
        HashUtility::hashCombine(outHash, keyval.idx);
        return outHash;
    }
};

class ExperimentalEnginePBR
    : public TestGameEngine
    , public IImGuiLayer
{
    SamplerRef nearestFiltering = nullptr;
    SamplerRef linearFiltering = nullptr;
    SamplerRef depthFiltering = nullptr;
    SamplerRef shadowFiltering = nullptr;
    // SamplerRef cubicFiltering = nullptr;
    void createImages(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyImages();
    // Asset's data
    std::unordered_map<ImageResourceRef, uint32> tex2dToBindlessIdx;
    // offset in count, in scene
    std::unordered_map<const MeshAsset *, std::pair<uint32, uint32>> meshVertIdxOffset;

    // Scene data
    // All used asset's vertex and index data
    BufferResourceRef sceneVertexBuffer;
    BufferResourceRef sceneIndexBuffer;
    BufferResourceRef allEntityDrawCmds;
    // Offset in bytes, Count in size
    std::unordered_map<const LocalPipelineContext *, std::pair<uint32, uint32>> pipelineToDrawCmdOffsetCount;
    std::array<BufferResourceRef, 8> spotDrawCmds;
    std::array<BufferResourceRef, 8> pointDrawCmds;
    void createDrawCmdsBuffer(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void setupLightSceneDrawCmdsBuffer(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance);
    void destroyDrawCmdsBuffer();

    std::vector<PBRSceneEntity> sceneData;

    std::vector<SpotLight> sceneSpotLights;
    std::vector<PointLight> scenePointLights;
    DirectionalLight dirLight;
    void sortSpotFromView(std::vector<uint32> &indices);
    void sortPointsFromView(std::vector<uint32> &indices);

    BoundingVolume<GridEntity> sceneVolume;
    GridEntity selection;

    // Now we support only 8 shadowed lights per type
    // Drawing light view
    std::array<ShaderParametersRef, 8> spotViewParams;
    std::array<ShaderParametersRef, 8> pointViewParams;
    ShaderParametersRef directionalViewParam;
    std::array<RenderTargetTexture *, 8> spotShadowRTs;
    std::array<RenderTargetTextureCube *, 8> pointShadowRTs;
    RenderTargetTextureArray *directionalShadowRT;
    uint32 shadowFlags;
    const float SHADOW_NEAR_PLANE = 0.05f;
    const float SHADOW_PLANE_MARGIN = 200.f;

    ShaderParametersRef lightDataShadowed;
    std::vector<ShaderParametersRef> lightData;
    void setupLightShadowViews();
    void setupCascadeShadowViews();
    void setupCascadeShadowViewsShimmerFix();
    ShaderParametersRef lightCommon;
    SwapchainBufferedResource<ShaderParametersRef> lightTextures;
    ShaderParametersRef viewParameters;
    ShaderParametersRef globalBindlessParameters;
    // We create instance data array such that all same mesh batch with same shader is in sequence so
    // that we can draw all those batches as an instance, Even if a mesh uses same shader, the material
    // is different so we have to create per batch
    //      sm1     sm2     sm3
    // B1   Mat1    Mat2    Mat1
    // B2   Mat2    Mat2    Mat2
    // Above table creates seq. as
    // I1       I2      I3      I4      I5      I6
    // M1S1B1  M1S3B1  M2S2B1  M2S1B2  M2S2B2  M2S3B2
    ShaderParametersRef instanceParameters;
    std::unordered_map<const LocalPipelineContext *, ShaderParametersRef> sceneShaderUniqParams;
    void createScene();
    void createSceneRenderData(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyScene();

    // Camera parameters
    Camera camera;
    Vector3D cameraTranslation;
    Rotation cameraRotation;
    void updateCameraParams();

    SwapchainBufferedResource<ShaderParametersRef> drawQuadTextureDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawQuadNormalDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawQuadAmbientDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawQuadRoughDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawQuadMetalDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawQuadDepthDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawLitColorsDescs;

    void createShaderParameters(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void setupShaderParameterParams(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyShaderParameters();

    void setupLightShaderData();
    void resizeLightingRts(const Size2D &size);
    void reupdateTextureParamsOnResize();
    void reupdateEnvMap();

    // Shader pipeline resources
    RenderPassClearValue clearValues;

    void createFrameResources(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyFrameResources();

    LocalPipelineContext singleColorPipelineContext;
    LocalPipelineContext texturedPipelineContext;

    LocalPipelineContext spotShadowPipelineContext;
    LocalPipelineContext directionalShadowPipelineContext;
    LocalPipelineContext pointShadowPipelineContext;

    LocalPipelineContext drawPbrWithShadowPipelineContext;
    LocalPipelineContext drawPbrNoShadowPipelineContext;

    LocalPipelineContext resolveToPresentPipelineContext;
    LocalPipelineContext overBlendedQuadPipelineContext;
    LocalPipelineContext resolveLightRtPipelineContext;

    ShaderParametersRef clearInfoParams;
    LocalPipelineContext clearQuadPipelineContext;

    LocalPipelineContext sceneDebugLinesPipelineContext;

    LocalPipelineContext drawLinesDWritePipelineCntxt;
    LocalPipelineContext drawGridDTestPipelineCntxt;

    // Gizmo drawing
    RenderTargetTexture *camGizmoColorTexture;
    RenderTargetTexture *camGizmoDepthTarget;
    void updateCamGizmoViewParams();
    ShaderParametersRef camViewAndInstanceParams;
    ShaderParametersRef camRTParams;

    void getPipelineContextForSubpass();
    void clearPipelineContexts();

    std::vector<FrameResource> frameResources;
    void createPipelineResources(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyPipelineResources();

    // End shader pipeline resources

    float exposure = 4.2f;
    float gamma = 2.2f;
    bool bDrawTbn = false;

    enum RenderFlags
    {
        DisableEnvAmbient = 1,
        DisableDirectional,
        DisableAmbNDir,
        DisableShadows,
        DrawCascade
    };
    int32 renderFlags = 0;
    bool bDrawGrid = false;
    float gridExtendSize = 500;
    float gridCellSize = 10;
    float cellMinPixelCoverage = 2;
    LinearColor thinColor = LinearColorConst::GRAY;
    LinearColor thickColor = LinearColorConst::WHITE;

    int32 frameVisualizeId = 0; // 0 color 1 normal 2 depth
    Size2D renderSize{ 1280, 720 };
    ECameraProjection projection = ECameraProjection::Perspective;

    // Textures
    std::vector<TextureAsset *> textures;
    std::vector<EnvironmentMapAsset *> envMaps;

    // Histogram data
    std::vector<std::string> textureNameStrs;
    std::vector<const AChar *> textureNames;
    int32 selectedTexture = 0;
    std::array<float, 32> histogram[3];

    // Env texture
    std::vector<std::string> envMapNameStrs;
    std::vector<const AChar *> envMapNames;
    int32 selectedEnv = 0;

    String noneString{ TCHAR("None") };

    IReferenceCollector *collector;
    cbe::Object *rootObj;
    cbe::Object *objectPtrTest;
    cbe::Object *objectPtrToValTest;
    cbe::Object *valToObjectPtrTest;
    cbe::Object *valToStructTest;
    cbe::Object *structToValTest;

protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

    void startUpRenderInit(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void renderQuit();
    void updateCamGizmoCapture(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance);
    void renderShadows(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsResource *cmdBuffer);
    void frameRender(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void debugFrameRender(
        class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsResource *cmdBuffer, uint32 swapchainIdx
    );

    void drawSelectionWidget(class ImGuiDrawInterface *drawInterface);

    void tempTest();
    void tempTestQuit();
    void tempTestPerFrame();
    /* IImGuiLayer Implementation */
public:
    int32 layerDepth() const override;
    int32 sublayerDepth() const override;
    void draw(class ImGuiDrawInterface *drawInterface) override;
    /* end overrides */

    AABB getBounds(const GridEntity &entity) const
    {
        switch (entity.type)
        {
        case GridEntity::Entity:
        {
            fatalAssertf(sceneData.size() > entity.idx, "Invalid index %d", entity.idx);
            AABB bound(
                sceneData[entity.idx].meshAsset->bounds.minBound * sceneData[entity.idx].transform.getScale()
                    + sceneData[entity.idx].transform.getTranslation(),
                sceneData[entity.idx].meshAsset->bounds.maxBound * sceneData[entity.idx].transform.getScale()
                    + sceneData[entity.idx].transform.getTranslation()
            );
            bound.fixAABB();
            return bound;
        }
        case GridEntity::PointLight:
        {
            fatalAssertf(scenePointLights.size() > entity.idx, "Invalid index %d", entity.idx);
            AABB bound(scenePointLights[entity.idx].lightPos - Vector3D(50), scenePointLights[entity.idx].lightPos + Vector3D(50));
            bound.fixAABB();
            return bound;
        }
        case GridEntity::SpotLight:
        {
            fatalAssertf(sceneSpotLights.size() > entity.idx, "Invalid index %d", entity.idx);
            AABB bound(
                sceneSpotLights[entity.idx].transform.getTranslation() - Vector3D(50),
                sceneSpotLights[entity.idx].transform.getTranslation() + Vector3D(50)
            );
            bound.fixAABB();
            return bound;
        }
        default:
            fatalAssertf(false, "Unsupported type");
            break;
        }
        return { Vector3D::ZERO, Vector3D::ZERO };
    }
};

AABB GridEntity::getBounds() const { return static_cast<ExperimentalEnginePBR *>(*gEngine)->getBounds(*this); }

void ExperimentalEnginePBR::createImages(IGraphicsInstance *, const GraphicsHelperAPI *)
{
    nearestFiltering = GlobalBuffers::nearestSampler();
    linearFiltering = GlobalBuffers::linearSampler();
    depthFiltering = GlobalBuffers::depthSampler();
    shadowFiltering = GlobalBuffers::shadowSampler();

    RenderTextureCreateParams rtCreateParams;
    rtCreateParams.bSameReadWriteTexture = true;
    rtCreateParams.bIsSrgb = false;
    rtCreateParams.format = ERenderTargetFormat::RT_U8Packed;
    rtCreateParams.textureSize = Size2D(256, 256);
    rtCreateParams.textureName = TCHAR("CameraGizmosRT");
    camGizmoColorTexture = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);

    rtCreateParams.format = ERenderTargetFormat::RT_Depth;
    camGizmoDepthTarget = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);

    // Shadow RTs
    Size2D baseDirRes(1024);
    RenderTextureArrayCreateParams directionalShadowRTCI;
    directionalShadowRTCI.bSameReadWriteTexture = true;
    directionalShadowRTCI.bIsSrgb = false;
    directionalShadowRTCI.format = ERenderTargetFormat::RT_Depth;
    directionalShadowRTCI.textureSize = baseDirRes;
    directionalShadowRTCI.layerCount = dirLight.cascadeCount;
    directionalShadowRTCI.textureName = TCHAR("CascadesRT");
    directionalShadowRT = TextureBase::createTexture<RenderTargetTextureArray>(directionalShadowRTCI);

    RenderTextureCreateParams lightShadowRtsCreateParam;
    lightShadowRtsCreateParam.bIsSrgb = false;
    lightShadowRtsCreateParam.format = ERenderTargetFormat::RT_Depth;
    lightShadowRtsCreateParam.bSameReadWriteTexture = true;
    lightShadowRtsCreateParam.textureSize = baseDirRes / Size2D(2);

    uint32 shadowRTCount = uint32(Math::min(spotShadowRTs.size(), sceneSpotLights.size()));
    for (uint32 i = 0; i < shadowRTCount; ++i)
    {
        lightShadowRtsCreateParam.textureName = TCHAR("SpotShadowRT_") + String::toString(i);
        spotShadowRTs[i] = TextureBase::createTexture<RenderTargetTexture>(lightShadowRtsCreateParam);
    }

    lightShadowRtsCreateParam.textureSize = baseDirRes / Size2D(4);
    shadowRTCount = uint32(Math::min(pointShadowRTs.size(), scenePointLights.size()));
    for (uint32 i = 0; i < shadowRTCount; ++i)
    {
        lightShadowRtsCreateParam.textureName = TCHAR("PointShadowRT_") + String::toString(i);
        pointShadowRTs[i] = TextureBase::createTexture<RenderTargetTextureCube>(lightShadowRtsCreateParam);
    }
}

void ExperimentalEnginePBR::destroyImages()
{
    nearestFiltering.reset();
    linearFiltering.reset();
    depthFiltering.reset();
    shadowFiltering.reset();

    TextureBase::destroyTexture<RenderTargetTexture>(camGizmoColorTexture);
    TextureBase::destroyTexture<RenderTargetTexture>(camGizmoDepthTarget);

    TextureBase::destroyTexture<RenderTargetTextureArray>(directionalShadowRT);
    for (RenderTargetTexture *rt : spotShadowRTs)
    {
        if (rt)
        {
            TextureBase::destroyTexture<RenderTargetTexture>(rt);
        }
    }
    for (RenderTargetTextureCube *rt : pointShadowRTs)
    {
        if (rt)
        {
            TextureBase::destroyTexture<RenderTargetTextureCube>(rt);
        }
    }
}

void ExperimentalEnginePBR::createDrawCmdsBuffer(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    // Setup all draw commands, Instance idx for each batch and its material idx
    std::vector<DrawIndexedIndirectCommand> drawCmds;
    {
        // Using set to sort by batch to use instanced draw
        std::unordered_map<LocalPipelineContext *, std::map<const MeshAsset *, std::set<std::pair<uint32, uint32>>>>
            pipelineToMeshToBatchEntityIdx;
        uint32 entityIdx = 0;
        for (PBRSceneEntity &entity : sceneData)
        {
            uint32 meshBatchIdx = 0;
            entity.instanceParamIdx.resize(entity.meshBatchProps.size());
            entity.batchShaderParamIdx.resize(entity.meshBatchProps.size());

            for (const PBRSceneEntity::BatchProperties &meshBatchProp : entity.meshBatchProps)
            {
                pipelineToMeshToBatchEntityIdx[meshBatchProp.pipeline][entity.meshAsset].insert(std::pair<uint32, uint32>{ meshBatchIdx,
                                                                                                                           entityIdx });
                ++meshBatchIdx;
            }
            entityIdx++;
        }

        uint32 totalDrawCalls = 0;
        uint32 instanceCount = 0; // For batch's instance idx
        // Insert draw calls and setup indices for both instances and materials
        for (const auto &pipeMeshPairToBatchEntity : pipelineToMeshToBatchEntityIdx)
        {
            uint32 pipelineDrawCalls = 0;
            uint32 materialCount = 0; // For batch's material idx
            for (const auto &meshAssetToBatchEntityIdx : pipeMeshPairToBatchEntity.second)
            {
                for (auto setItr = meshAssetToBatchEntityIdx.second.cbegin(); setItr != meshAssetToBatchEntityIdx.second.cend();)
                {
                    // set material and instance index for a batch
                    sceneData[setItr->second].instanceParamIdx[setItr->first] = instanceCount;
                    sceneData[setItr->second].batchShaderParamIdx[setItr->first] = materialCount;
                    instanceCount++;
                    materialCount++;

                    auto nextItr = setItr;
                    nextItr++;
                    // Go fwd until different batch or end is reached
                    while (nextItr != meshAssetToBatchEntityIdx.second.cend() && nextItr->first == setItr->first)
                    {
                        // set material and instance index for a batch
                        sceneData[nextItr->second].instanceParamIdx[nextItr->first] = instanceCount;
                        sceneData[nextItr->second].batchShaderParamIdx[nextItr->first] = materialCount;
                        instanceCount++;
                        materialCount++;

                        nextItr++;
                    }
                    const MeshVertexView &meshBatch
                        = static_cast<const StaticMeshAsset *>(meshAssetToBatchEntityIdx.first)->meshBatches[setItr->first];
                    // fill draw command for this batch
                    DrawIndexedIndirectCommand &drawCmd = drawCmds.emplace_back();
                    drawCmd.firstInstance = sceneData[setItr->second].instanceParamIdx[setItr->first];
                    drawCmd.firstIndex = meshVertIdxOffset[meshAssetToBatchEntityIdx.first].second // Mesh's scene index buffer offset
                                         + meshBatch.startIndex;                                   // Local index buffer offset
                    drawCmd.indexCount = meshBatch.numOfIndices;
                    drawCmd.instanceCount = instanceCount - drawCmd.firstInstance;
                    drawCmd.vertexOffset = meshVertIdxOffset[meshAssetToBatchEntityIdx.first].first;

                    setItr = nextItr;
                    pipelineDrawCalls++;
                }
            }
            // Setting draw cmd buffer offsets for this pipeline
            pipelineToDrawCmdOffsetCount[pipeMeshPairToBatchEntity.first]
                = std::pair<uint32, uint32>{ uint32(totalDrawCalls * sizeof(DrawIndexedIndirectCommand)), pipelineDrawCalls };
            // Resizing material parameters
            sceneShaderUniqParams[pipeMeshPairToBatchEntity.first]->resizeRuntimeBuffer(TCHAR("materials"), materialCount);
            totalDrawCalls += pipelineDrawCalls;
            LOG("ExperimentalEnginePBR", "%s Pipeline's Material's count %d", pipeMeshPairToBatchEntity.first->materialName, materialCount);
            LOG("ExperimentalEnginePBR", "%s Pipeline's instanced draw calls %d", pipeMeshPairToBatchEntity.first->materialName,
                pipelineDrawCalls);
        }
        LOG("ExperimentalEnginePBR", "Total instanced draw calls %d", totalDrawCalls);

        // Resize instance parameters
        instanceParameters->resizeRuntimeBuffer(TCHAR("instancesWrapper"), instanceCount);

        // Create buffer with draw calls and copy draw cmds
        allEntityDrawCmds = graphicsHelper->createReadOnlyIndirectBuffer(graphicsInstance, sizeof(DrawIndexedIndirectCommand), totalDrawCalls);
        allEntityDrawCmds->setResourceName(TCHAR("AllEntityDrawCmds"));
        allEntityDrawCmds->init();

        // Now setup instance and material parameters
        for (PBRSceneEntity &entity : sceneData)
        {
            uint32 meshBatchIdx = 0;
            for (const PBRSceneEntity::BatchProperties &meshBatchProp : entity.meshBatchProps)
            {
                entity.updateInstanceParams(instanceParameters, meshBatchIdx);
                entity.updateMaterialParams(sceneShaderUniqParams[meshBatchProp.pipeline], tex2dToBindlessIdx, meshBatchIdx);
                ++meshBatchIdx;
            }
            entityIdx++;
        }
    }

    // TODO(Jeslas) : Not doing per light culling as it is faster without it, Enable after adding
    // gpu/compute culling
    for (uint32 i = 0; i < pointShadowRTs.size() && pointShadowRTs[i]; ++i)
    {
        pointDrawCmds[i] = graphicsHelper->createReadOnlyIndirectBuffer(graphicsInstance, sizeof(DrawIndexedIndirectCommand));
        pointDrawCmds[i]->setAsStagingResource(true);
        pointDrawCmds[i]->setDeferredDelete(false);
        pointDrawCmds[i]->setResourceName(TCHAR("PointDepthDrawCmds_") + String::toString(i));
#if SHADOWS_USE_CULLED_DRAW_CMDS
        pointDrawCmds[i]->init();
#endif
    }
    for (uint32 i = 0; i < spotShadowRTs.size() && spotShadowRTs[i]; ++i)
    {
        spotDrawCmds[i] = graphicsHelper->createReadOnlyIndirectBuffer(graphicsInstance, sizeof(DrawIndexedIndirectCommand));
        spotDrawCmds[i]->setAsStagingResource(true);
        spotDrawCmds[i]->setDeferredDelete(false);
        spotDrawCmds[i]->setResourceName(TCHAR("SpotDepthDrawCmds_") + String::toString(i));
#if SHADOWS_USE_CULLED_DRAW_CMDS
        spotDrawCmds[i]->init();
#endif
    }
    ENQUEUE_COMMAND(CreateAllEntityDrawCmds)
    (
        [drawCmds, this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *)
        {
            cmdList->copyToBuffer(allEntityDrawCmds, 0, drawCmds.data(), uint32(allEntityDrawCmds->getResourceSize()));
        // TODO(Jeslas) : Not doing per light culling as it is faster without it, Enable after adding
        // gpu/compute culling
#if SHADOWS_USE_CULLED_DRAW_CMDS
            setupLightSceneDrawCmdsBuffer(cmdList, graphicsInstance);
#endif
        }
    );
}

void PBRSceneEntity::updateInstanceParams(ShaderParametersRef &shaderParams, uint32 batchIdx)
{
    InstanceData gpuInstance;
    gpuInstance.model = transform.getTransformMatrix();
    gpuInstance.invModel = gpuInstance.model.inverse();
    gpuInstance.invModel = transform.inverseNonUniformScaled().getTransformMatrix();
    gpuInstance.shaderUniqIdx = batchShaderParamIdx[batchIdx];

    // shaderParams->setBuffer(TCHAR("instances"), gpuInstance, instanceParamIdx[batchIdx]);

    // Below code to test set by path and indices
    StringID paramPath[] = { STRID("instancesWrapper"), STRID("instances"), STRID("model") };
    uint32 paramIndices[] = { 0, instanceParamIdx[batchIdx], 0 };
    shaderParams->setMatrixAtPath(paramPath, paramIndices, gpuInstance.model);
    paramPath[2] = STRID("invModel");
    shaderParams->setMatrixAtPath(paramPath, paramIndices, gpuInstance.model);
    paramPath[2] = STRID("shaderUniqIdx");
    shaderParams->setIntAtPath(paramPath, paramIndices, batchShaderParamIdx[batchIdx]);
}

void PBRSceneEntity::updateMaterialParams(
    ShaderParametersRef &shaderParams, const std::unordered_map<ImageResourceRef, uint32> &tex2dToBindlessIdx, uint32 batchIdx
) const
{
    const BatchProperties &meshBatch = meshBatchProps[batchIdx];

    SingleColorMeshData singleColorMeshData;
    singleColorMeshData.meshColor = meshBatch.color;
    singleColorMeshData.metallic = meshBatch.metallic;
    singleColorMeshData.roughness = meshBatch.roughness;
    if (!shaderParams->setBuffer(TCHAR("meshData"), singleColorMeshData, batchShaderParamIdx[batchIdx]))
    {
        TexturedMeshData texturedMeshData;
        texturedMeshData.meshColor = meshBatch.color;
        texturedMeshData.rm_uvScale = { meshBatch.roughness, meshBatch.metallic, meshBatch.uvScale.x(), meshBatch.uvScale.y() };
        texturedMeshData.diffuseMapIdx
            = (tex2dToBindlessIdx.find(static_cast<TextureAsset *>(gEngine->getAssetManager().getAsset(meshBatch.textureName + TCHAR("_D")))
                                                                       ->getTexture()
                                                                       ->getTextureResource())
                                           ->second);
        texturedMeshData.normalMapIdx
            = (tex2dToBindlessIdx.find(static_cast<TextureAsset *>(gEngine->getAssetManager().getAsset(meshBatch.textureName + TCHAR("_N")))
                                                                       ->getTexture()
                                                                       ->getTextureResource())
                                           ->second);
        texturedMeshData.armMapIdx
            = (tex2dToBindlessIdx.find(static_cast<TextureAsset *>(gEngine->getAssetManager().getAsset(meshBatch.textureName + TCHAR("_ARM")))
                                                                       ->getTexture()
                                                                       ->getTextureResource())
                                           ->second);
        shaderParams->setBuffer(TCHAR("meshData"), texturedMeshData, batchShaderParamIdx[batchIdx]);
    }
}

void ExperimentalEnginePBR::setupLightSceneDrawCmdsBuffer(class IRenderCommandList *cmdList, IGraphicsInstance *)
{
    struct LightObjectCulling
    {
        std::vector<DrawIndexedIndirectCommand> drawCmds;
        std::vector<GridEntity> setIntersections;
    };
    std::vector<LightObjectCulling> lightCullings(sceneSpotLights.size() + scenePointLights.size());
    bool bHasAnyBufferResize = false;
    auto cullLambda = [&lightCullings, &bHasAnyBufferResize, this](uint32 idx, bool bIsPtLights)
    {
        BufferResourceRef drawCmdsBuffer = nullptr;
        uint32 lightCullingOffset = 0;
        AABB lightRegion;
        if (bIsPtLights)
        {
            lightCullingOffset = uint32(sceneSpotLights.size());
            const PointLight &ptlit = scenePointLights[idx];
            if (!ptlit.shadowViewParams || !ptlit.shadowMap || !ptlit.drawCmdsBuffer)
            {
                return;
            }
            lightRegion = AABB(ptlit.lightPos + Vector3D(ptlit.radius, 0, 0));
            lightRegion.grow(ptlit.lightPos + Vector3D(-ptlit.radius, 0, 0));
            lightRegion.grow(ptlit.lightPos + Vector3D(0, ptlit.radius, 0));
            lightRegion.grow(ptlit.lightPos + Vector3D(0, -ptlit.radius, 0));
            lightRegion.grow(ptlit.lightPos + Vector3D(0, 0, ptlit.radius));
            lightRegion.grow(ptlit.lightPos + Vector3D(0, 0, -ptlit.radius));

            drawCmdsBuffer = *ptlit.drawCmdsBuffer;
        }
        else
        {
            const SpotLight &sptlit = sceneSpotLights[idx];
            if (!sptlit.shadowViewParams || !sptlit.shadowMap || !sptlit.drawCmdsBuffer)
            {
                return;
            }

            Vector3D corners[8];
            sptlit.view.frustumCorners(corners);
            ArrayView<Vector3D> cornersView(corners);
            lightRegion = AABB(cornersView);

            drawCmdsBuffer = *sptlit.drawCmdsBuffer;
        }

        LightObjectCulling &lightCulling = lightCullings[lightCullingOffset + idx];
        sceneVolume.findIntersection(lightCulling.setIntersections, lightRegion, true);
        for (const GridEntity &gridEntity : lightCulling.setIntersections)
        {
            if (gridEntity.type == GridEntity::Entity)
            {
                const PBRSceneEntity &sceneEntity = sceneData[gridEntity.idx];
                for (SizeT meshBatchIdx = 0; meshBatchIdx < sceneEntity.meshBatchProps.size(); ++meshBatchIdx)
                {
                    const MeshVertexView &meshBatch = static_cast<const StaticMeshAsset *>(sceneEntity.meshAsset)->meshBatches[meshBatchIdx];
                    // fill draw command for this batch
                    DrawIndexedIndirectCommand &drawCmd = lightCulling.drawCmds.emplace_back();
                    drawCmd.firstInstance = sceneEntity.instanceParamIdx[meshBatchIdx];
                    drawCmd.firstIndex = meshVertIdxOffset[sceneEntity.meshAsset].second // Mesh's scene index buffer offset
                                         + meshBatch.startIndex;                         // Local index buffer offset
                    drawCmd.indexCount = meshBatch.numOfIndices;
                    drawCmd.instanceCount = 1;
                    drawCmd.vertexOffset = meshVertIdxOffset[sceneEntity.meshAsset].first;
                }
            }
        }

        bHasAnyBufferResize = bHasAnyBufferResize || (drawCmdsBuffer->bufferCount() < uint32(lightCulling.drawCmds.size()));
        if (bIsPtLights)
        {
            scenePointLights[idx].drawCmdCount = uint32(lightCulling.drawCmds.size());
        }
        else
        {
            sceneSpotLights[idx].drawCmdCount = uint32(lightCulling.drawCmds.size());
        }
    };

    auto sceneSptCullingJobs = copat::dispatch(
        copat::JobSystem::get(), copat::DispatchFunctionType::createLambda(cullLambda, false), uint32(sceneSpotLights.size())
    );
    auto scenePtCullingJobs = copat::dispatch(
        copat::JobSystem::get(), copat::DispatchFunctionType::createLambda(cullLambda, true), uint32(scenePointLights.size())
    );
    copat::waitOnAwaitable(sceneSptCullingJobs);
    copat::waitOnAwaitable(scenePtCullingJobs);

    if (bHasAnyBufferResize)
    {
        cmdList->flushAllcommands();
    }
    std::vector<BatchCopyBufferData> batchCopies;
    for (SizeT i = 0; i < sceneSpotLights.size(); ++i)
    {
        const LightObjectCulling &lightCulling = lightCullings[i];
        SpotLight &sptlit = sceneSpotLights[i];
        if (!sptlit.drawCmdsBuffer)
        {
            continue;
        }
        if ((*sptlit.drawCmdsBuffer)->bufferCount() < lightCulling.drawCmds.size())
        {
            (*sptlit.drawCmdsBuffer)->setBufferCount(uint32(lightCulling.drawCmds.size()));
            (*sptlit.drawCmdsBuffer)->reinitResources();
        }

        batchCopies.emplace_back(BatchCopyBufferData{ .dst = (*sptlit.drawCmdsBuffer),
                                                      .dstOffset = 0,
                                                      .dataToCopy = lightCulling.drawCmds.data(),
                                                      .size = uint32((*sptlit.drawCmdsBuffer)->getResourceSize()) });
    }
    for (SizeT i = 0; i < scenePointLights.size(); ++i)
    {
        const LightObjectCulling &lightCulling = lightCullings[sceneSpotLights.size() + i];
        PointLight &ptlit = scenePointLights[i];
        if (!ptlit.drawCmdsBuffer)
        {
            continue;
        }
        if ((*ptlit.drawCmdsBuffer)->bufferCount() < uint32(lightCulling.drawCmds.size()))
        {
            (*ptlit.drawCmdsBuffer)->setBufferCount(uint32(lightCulling.drawCmds.size()));
            (*ptlit.drawCmdsBuffer)->reinitResources();
        }

        batchCopies.emplace_back(BatchCopyBufferData{ .dst = (*ptlit.drawCmdsBuffer),
                                                      .dstOffset = 0,
                                                      .dataToCopy = lightCulling.drawCmds.data(),
                                                      .size = uint32((*ptlit.drawCmdsBuffer)->getResourceSize()) });
    }
    cmdList->copyToBuffer(batchCopies);
}

void ExperimentalEnginePBR::destroyDrawCmdsBuffer()
{
    allEntityDrawCmds.reset();

    for (uint32 i = 0; i < pointShadowRTs.size() && pointShadowRTs[i]; ++i)
    {
        if (pointDrawCmds[i].isValid())
        {
            pointDrawCmds[i].reset();
        }
    }
    for (uint32 i = 0; i < spotShadowRTs.size() && spotShadowRTs[i]; ++i)
    {
        if (spotDrawCmds[i].isValid())
        {
            spotDrawCmds[i].reset();
        }
    }
}

void ExperimentalEnginePBR::sortSpotFromView(std::vector<uint32> &indices)
{
    indices.resize(sceneSpotLights.size());

    for (int32 i = 0; i < indices.size(); ++i)
    {
        indices[i] = i;
    }

    std::sort(
        indices.begin(), indices.end(),
        [this](uint32 lhs, uint32 rhs)
        {
            const Vector3D lhsLen = (sceneSpotLights[lhs].transform.getTranslation() - camera.translation());
            const Vector3D rhsLen = (sceneSpotLights[rhs].transform.getTranslation() - camera.translation());

            return (lhsLen | lhsLen) < (rhsLen | rhsLen);
        }
    );
}

void ExperimentalEnginePBR::sortPointsFromView(std::vector<uint32> &indices)
{
    indices.resize(scenePointLights.size());

    for (int32 i = 0; i < indices.size(); ++i)
    {
        indices[i] = i;
    }

    std::sort(
        indices.begin(), indices.end(),
        [this](uint32 lhs, uint32 rhs)
        {
            const Vector3D lhsLen = (scenePointLights[lhs].lightPos - camera.translation());
            const Vector3D rhsLen = (scenePointLights[rhs].lightPos - camera.translation());

            return (lhsLen | lhsLen) < (rhsLen | rhsLen);
        }
    );
}

void ExperimentalEnginePBR::setupLightShadowViews()
{
    for (SpotLight &spotL : sceneSpotLights)
    {
        spotL.view.setRotation(spotL.transform.getRotation());
        spotL.view.setTranslation(spotL.transform.getTranslation());
        spotL.view.cameraProjection = ECameraProjection::Perspective;
        spotL.view.setFOV(spotL.outerCone, spotL.outerCone);
        spotL.view.setClippingPlane(SHADOW_NEAR_PLANE, spotL.radius + SHADOW_PLANE_MARGIN);
    }
    for (PointLight &ptL : scenePointLights)
    {
        uint32 idx = 0;
        for (Camera &view : ptL.views)
        {
            view.setTranslation(ptL.lightPos);
            view.setRotation(PointShadowDepthViews::VIEW_DIRECTIONS[idx]);
            view.cameraProjection = ECameraProjection::Perspective;
            view.setFOV(90, 90);
            view.setClippingPlane(SHADOW_NEAR_PLANE, ptL.radius + SHADOW_PLANE_MARGIN);

            idx++;
        }
    }
    setupCascadeShadowViewsShimmerFix();
}

void ExperimentalEnginePBR::setupCascadeShadowViews()
{
    // Directional light cascades
    const AABB sceneBounds = sceneVolume.getBounds();
    Vector3D sceneBoundPts[8];
    ArrayView<Vector3D> sceneBoundPtsView(sceneBoundPts, ARRAY_LENGTH(sceneBoundPts));
    sceneBounds.boundCorners(sceneBoundPtsView);

    const Matrix3 dirLightToWorld = RotationMatrix::fromX(dirLight.direction.fwdVector()).matrix();
    const Matrix3 worldToDirLight = dirLightToWorld.transpose(); // Since it is orthogonal matrix
    const Vector3D dirLightFwd = dirLight.direction.fwdVector();

    Camera tempCamera = camera;
    tempCamera.setClippingPlane(camera.nearPlane(), camera.farPlane() * dirLight.cascades[0].frustumFract);
    for (uint32 i = 0; i < dirLight.cascadeCount; ++i)
    {
        // Finding view orthographic size
        AABB box(Vector3D(FLT_MAX), Vector3D(FLT_MIN));
        std::array<Vector3D, 8> corners;
        tempCamera.frustumCorners(corners.data());
        for (const Vector3D &corner : corners)
        {
            box.grow(worldToDirLight * corner);
        }
        Vector3D extend = box.size();
        Vector3D center = dirLightToWorld * box.center();

        // To determine the near and far plane so that they would cover all level objects
        ValueRange<float> nearFarValues(FLT_MAX, FLT_MIN);
        for (int32 boundIdx = 0; boundIdx < ARRAY_LENGTH(sceneBoundPts); ++boundIdx)
        {
            nearFarValues.grow((sceneBoundPts[boundIdx] - center) | dirLightFwd);
        }

        dirLight.cascades[i].cascadeView.cameraProjection = ECameraProjection::Orthographic;
        dirLight.cascades[i].cascadeView.setRotation(RotationMatrix::fromX(dirLightFwd).asRotation());
        dirLight.cascades[i].cascadeView.setTranslation(
            center + dirLightFwd * (nearFarValues.minBound - SHADOW_NEAR_PLANE - SHADOW_PLANE_MARGIN)
        );
        // Since Y, Z will be X, Y of surface
        dirLight.cascades[i].cascadeView.setOrthoSize({ extend.y(), extend.z() });
        dirLight.cascades[i].cascadeView.setClippingPlane(SHADOW_NEAR_PLANE, nearFarValues.size() + SHADOW_NEAR_PLANE + SHADOW_PLANE_MARGIN);
        dirLight.cascades[i].frustumFarDistance = tempCamera.farPlane();

        tempCamera.setClippingPlane(
            tempCamera.farPlane(), tempCamera.farPlane() + camera.farPlane() * dirLight.cascades[i].frustumFract + SHADOW_PLANE_MARGIN
        );
    }
}

void ExperimentalEnginePBR::setupCascadeShadowViewsShimmerFix()
{
    // Directional light cascades
    const AABB sceneBounds = sceneVolume.getBounds();
    Vector3D sceneBoundPts[8];
    ArrayView<Vector3D> sceneBoundPtsView(sceneBoundPts);
    sceneBounds.boundCorners(sceneBoundPtsView);

    const Vector3D dirLightFwd = dirLight.direction.fwdVector();
    Camera tempCamera = camera;
    tempCamera.setClippingPlane(camera.nearPlane(), camera.farPlane() * dirLight.cascades[0].frustumFract);
    for (uint32 i = 0; i < dirLight.cascadeCount; ++i)
    {
        // Finding view orthographic size
        std::array<Vector3D, 8> corners;
        Vector3D center;
        tempCamera.frustumCorners(corners.data(), &center);
        // Using sphere bounds to fix rotational shimmering
        float frustumMaxRadius = 0;
        for (const Vector3D &corner : corners)
        {
            frustumMaxRadius = Math::max(frustumMaxRadius, (corner - center).length());
        }
        frustumMaxRadius = Math::ceil(frustumMaxRadius * 16.0f) / 16.0f;

        // To determine the near and far plane so that they would cover all level objects
        ValueRange<float> nearFarValues(FLT_MAX, FLT_MIN);
        for (int32 boundIdx = 0; boundIdx < ARRAY_LENGTH(sceneBoundPts); ++boundIdx)
        {
            nearFarValues.grow((sceneBoundPts[boundIdx] - center) | dirLightFwd);
        }

        dirLight.cascades[i].cascadeView.cameraProjection = ECameraProjection::Orthographic;
        dirLight.cascades[i].cascadeView.setRotation(RotationMatrix::fromX(dirLightFwd).asRotation());
        dirLight.cascades[i].cascadeView.setTranslation(
            center + dirLightFwd * (nearFarValues.minBound - SHADOW_NEAR_PLANE - SHADOW_PLANE_MARGIN)
        );
        dirLight.cascades[i].cascadeView.setOrthoSize({ 2 * frustumMaxRadius, 2 * frustumMaxRadius });
        dirLight.cascades[i].cascadeView.setClippingPlane(SHADOW_NEAR_PLANE, nearFarValues.size() + SHADOW_NEAR_PLANE + SHADOW_PLANE_MARGIN);
        dirLight.cascades[i].frustumFarDistance = tempCamera.farPlane();

        // From
        // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps
        // - Does not make sense Shimmering issue - fix from
        // https://jcoluna.wordpress.com/2011/07/06/xna-light-pre-pass-cascade-shadow-maps/ - Did not
        // fix, try exposing l,r,b,t ortho constructor to check further Shimmering issue - fix from
        // https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/ or
        // https://therealmjp.github.io/posts/shadow-maps/ or
        // https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering
        Matrix4 projMatrix = dirLight.cascades[i].cascadeView.projectionMatrix();
        Matrix4 shadowMatrix = projMatrix * dirLight.cascades[i].cascadeView.viewMatrix().inverse();
        // No divide by W as this is orthographic projection
        Vector3D shadowOrigin(shadowMatrix * Vector4D(Vector3D::ZERO, 1.0f));
        shadowOrigin *= directionalShadowRT->getTextureSize().x / 2.0f;
        Vector3D roundedOrigin = Math::round(shadowOrigin);
        // In projected clip space
        Vector3D roundedOffset = roundedOrigin - shadowOrigin;
        roundedOffset *= 2.0f / directionalShadowRT->getTextureSize().x;
        projMatrix[3].x += roundedOffset.x();
        projMatrix[3].y += roundedOffset.y();
        dirLight.cascades[i].cascadeView.setCustomProjection(projMatrix);
        // In World space
        // roundedOrigin.z() = shadowOrigin.z();
        // roundedOrigin *= 2.0f / directionalShadowRT->getTextureSize().x;
        // Matrix4 shadowClipToWorld = dirLight.cascades[i].cascadeView.viewMatrix() *
        // projMatrix.inverse(); roundedOrigin = shadowClipToWorld * Vector4D(roundedOrigin, 1.0f);
        // dirLight.cascades[i].cascadeView.setTranslation(dirLight.cascades[i].cascadeView.translation()
        // + roundedOrigin);

        tempCamera.setClippingPlane(
            tempCamera.farPlane(), tempCamera.farPlane() + camera.farPlane() * dirLight.cascades[i].frustumFract + SHADOW_PLANE_MARGIN
        );
    }
}

void ExperimentalEnginePBR::createScene()
{
    StaticMeshAsset *cube = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Cube.obj")));
    // StaticMeshAsset* plane =
    // static_cast<StaticMeshAsset*>(assetManager.getOrLoadAsset(TCHAR("Plane.obj")));
    StaticMeshAsset *sphere = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Sphere.obj")));
    StaticMeshAsset *cylinder = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Cylinder.obj")));
    StaticMeshAsset *cone = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Cone.obj")));
    StaticMeshAsset *suzanne = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Suzanne.obj")));
    std::array<StaticMeshAsset *, 5> assets{ cube, sphere, cylinder, cone, suzanne };

// clang-format off
#if RELEASE_BUILD
    std::array<String, 8> floorTypes{
        TCHAR("WoodFloor043"), TCHAR("Tiles086"),  TCHAR("Tiles074"),  TCHAR("MetalPlates006"),
        TCHAR("Marble006"),    TCHAR("Ground042"), TCHAR("Ground037"), TCHAR("Gravel022") };
    std::array<String, 6> ceilTypes{
        TCHAR("WoodFloor043"), TCHAR("Tiles108"), TCHAR("Tiles074"), TCHAR("MetalPlates006"), TCHAR("Marble006"), TCHAR("Wood051") };
    std::array<String, 9> pillarTypes{
        TCHAR("WoodFloor043"), TCHAR("Tiles108"), TCHAR("Tiles074"),  TCHAR("MetalPlates006"),   TCHAR("Marble006"),
        TCHAR("Marble006"),    TCHAR("Rock035"),  TCHAR("Ground037"), TCHAR("PaintedPlaster016") };
    std::array<String, 15> meshTextures{ TCHAR("Bricks059"),      TCHAR("Gravel022"),         TCHAR("Ground037"), TCHAR("Ground042"),
                                     TCHAR("Leather028"),     TCHAR("Marble006"),         TCHAR("Metal034"),  TCHAR("Metal038"),
                                     TCHAR("MetalPlates006"), TCHAR("PaintedPlaster016"), TCHAR("Rock035"),   TCHAR("Tiles086"),
                                     TCHAR("Tiles074"),       TCHAR("Tiles108"),          TCHAR("Wood051") };
#else
    std::array<String, 1> floorTypes{ TCHAR("Tiles074") };
    std::array<String, 1> ceilTypes{ TCHAR("Tiles074") };
    std::array<String, 1> pillarTypes{ TCHAR("Tiles074") };
    std::array<String, 1> meshTextures{ TCHAR("Tiles074") };
#endif
    // clang-format on

    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    std::uniform_real_distribution<float> ud01(0.0, 1.0);
    std::normal_distribution<float> distribution1(0.5f, 0.15f);

    const Vector2D floorTextureScale(1 / 16.0f);
    const Vector2D pillarTextureScale(1 / 3.0f, 1 / 6.0f);
    const Vector2D textureScale(1 / 3.0f);

    std::vector<GridEntity> entities;
    auto pushEntity = [&entities, this](const PBRSceneEntity &entity)
    {
        entities.emplace_back(GridEntity{ GridEntity::Entity, uint32(sceneData.size()) });
        sceneData.emplace_back(entity);
    };
    auto pushSpt = [&entities, this](const SpotLight &spotLight)
    {
        entities.emplace_back(GridEntity{ GridEntity::SpotLight, uint32(sceneSpotLights.size()) });
        sceneSpotLights.emplace_back(spotLight);
    };
    auto pushPt = [&entities, this](const PointLight &pointLight)
    {
        entities.emplace_back(GridEntity{ GridEntity::PointLight, uint32(scenePointLights.size()) });
        scenePointLights.emplace_back(pointLight);
    };

    const int32 halfCount = 1;
    for (int32 i = -halfCount; i <= halfCount; i++)
    {
        for (int32 j = -halfCount; j <= halfCount; j++)
        {
            String roomIdx = String::toString((i + 1) * 3 + j + 1);
            Vector3D offset = Vector3D(i * 1400.0f, j * 1400.0f, 0);
            PBRSceneEntity sceneFloor;
            sceneFloor.meshAsset = cube;
            sceneFloor.transform.setScale(Vector3D(13, 13, 1));
            sceneFloor.transform.setTranslation(offset + Vector3D(0, 0, -45));
            sceneFloor.name = TCHAR("floor") + roomIdx;

            for (uint32 batchIdx = 0; batchIdx < sceneFloor.meshAsset->meshBatches.size(); ++batchIdx)
            {
                sceneFloor.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties{
                    //{ ud01(generator) * 0.75f, ud01(generator) * 0.75f, ud01(generator) *
                    // 0.75f, 1 }
                    LinearColorConst::WHITE, 1.f, 1.f, floorTextureScale, floorTypes[uint32(floorTypes.size() * ud01(generator))],
                    &texturedPipelineContext });
            }
            pushEntity(sceneFloor);

            dirLight.direction.pitch() = 35;
            dirLight.direction.yaw() = 45;
            dirLight.lumen = 100;
            dirLight.lightcolor = LinearColor(1.f, 1.f, 0.8f);

            if (i == 0 && j == 0)
            {
                for (uint32 m = 0; m < 10; m++)
                {
                    for (uint32 r = 0; r < 10; r++)
                    {
                        float rough = (r * 0.1f) + 0.05f;
                        float metallic = (m * 0.1f) + 0.05f;
                        String suffix = TCHAR("_R_") + String::toString(r) + TCHAR("_M_") + String::toString(m);

                        Vector3D pos = offset + Vector3D(65.f + m * 130.0f, 65.f + r * 130.f, 25.f) - Vector3D(650, 650, 0);

                        PBRSceneEntity entity;
                        entity.transform.setTranslation(pos + Vector3D(0, 0, 75));
                        entity.meshAsset = sphere;
                        entity.name = sphere->assetName() + suffix;

                        for (uint32 batchIdx = 0; batchIdx < entity.meshAsset->meshBatches.size(); ++batchIdx)
                        {
                            entity.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties{
                                {0.5f, 0.0f, 0.0f},
                                rough,
                                metallic,
                                textureScale,
                                meshTextures[uint32(meshTextures.size() * ud01(generator))],
                                &singleColorPipelineContext
                            });
                        }
                        pushEntity(entity);

                        entity.meshAsset = cube;
                        entity.name = cube->assetName() + suffix;
                        entity.meshBatchProps.clear();
                        for (uint32 batchIdx = 0; batchIdx < entity.meshAsset->meshBatches.size(); ++batchIdx)
                        {
                            entity.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties{
                                {0.5f, 0.0f, 0.0f},
                                rough,
                                metallic,
                                textureScale,
                                meshTextures[uint32(meshTextures.size() * ud01(generator))],
                                &singleColorPipelineContext
                            });
                        }
                        entity.transform.setTranslation(pos);
                        entity.transform.setScale(Vector3D(1, 1, 0.5));
                        pushEntity(entity);
                    }
                }
                // Lights
                //{
                //    PointLight light;
                //    light.radius = 700;
                //    light.lumen = 250;
                //    light.lightcolor = LinearColorConst::WHITE;

                //    light.lightPos = offset + Vector3D(250, 250, 250);
                //    light.name = TCHAR("point0_") + roomIdx;
                //    pushPt(light);

                //    light.lightPos = offset + Vector3D(250, -250, 250);
                //    light.name = TCHAR("point1_") + roomIdx;
                //    pushPt(light);

                //    light.lightPos = offset + Vector3D(-250, 250, 250);
                //    light.name = TCHAR("point2_") + roomIdx;
                //    pushPt(light);

                //    light.lightPos = offset + Vector3D(-250, -250, 250);
                //    light.name = TCHAR("point3_") + roomIdx;
                //    pushPt(light);
                //}
            }
            else
            {
                // Ceiling
                for (auto &batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = ceilTypes[uint32(ceilTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(0, 0, 550));
                sceneFloor.name = TCHAR("ceil") + roomIdx;
                pushEntity(sceneFloor);

                for (uint32 n = 0; n < 5; ++n)
                {
                    PBRSceneEntity entity;
                    entity.meshAsset = assets[std::rand() % assets.size()];
                    entity.transform.setTranslation(
                        offset + Vector3D(distribution(generator) * 400, distribution(generator) * 400, distribution1(generator) * 100 + 50)
                    );
                    entity.transform.setRotation(Rotation(0, 0, distribution(generator) * 45));
                    entity.name = entity.meshAsset->assetName() + roomIdx + TCHAR("_") + String::toString(n);

                    for (uint32 batchIdx = 0; batchIdx < entity.meshAsset->meshBatches.size(); ++batchIdx)
                    {
                        entity.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties{
                            LinearColorConst::WHITE, 1.0f, 1.0f, textureScale, meshTextures[uint32(meshTextures.size() * ud01(generator))],
                            &texturedPipelineContext });
                    }
                    pushEntity(entity);
                }

                // Near floor
                float height = 175;
                if (ud01(generator) > 0.5f)
                {
                    SpotLight light;
                    light.radius = 700;
                    light.innerCone = 60;
                    light.outerCone = 80;
                    light.lumen = 200;
                    light.transform.setTranslation(offset + Vector3D(0, 0, height));

                    Vector3D dir = Vector3D(distribution(generator), distribution(generator), -0.5);
                    light.name = TCHAR("spot0_") + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    light.transform.setRotation(RotationMatrix::fromX(dir).asRotation());
                    pushSpt(light);

                    dir = dir * Vector3D(-1, -1, 1);
                    light.name = TCHAR("spot1_") + roomIdx;
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
                    light.name = TCHAR("point0_") + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);

                    light.lightPos = offset + Vector3D(400, -400, 130);
                    light.name = TCHAR("point1_") + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);

                    light.lightPos = offset + Vector3D(-400, 400, 130);
                    light.name = TCHAR("point2_") + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);

                    light.lightPos = offset + Vector3D(-400, -400, 130);
                    light.name = TCHAR("point3_") + roomIdx;
                    light.lightcolor = LinearColor(distribution1(generator), distribution1(generator), distribution1(generator), 1);
                    pushPt(light);
                }

                // Pillars
                for (auto &batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.uvScale = pillarTextureScale;
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.meshAsset = cylinder;
                sceneFloor.transform.setScale(Vector3D(1, 1, 5));
                sceneFloor.transform.setTranslation(offset + Vector3D(450, 450, 250));
                sceneFloor.name = TCHAR("pillar1_") + roomIdx;
                pushEntity(sceneFloor);

                for (auto &batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(-450, 450, 250));
                sceneFloor.name = TCHAR("pillar2_") + roomIdx;
                pushEntity(sceneFloor);

                for (auto &batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(450, -450, 250));
                sceneFloor.name = TCHAR("pillar3_") + roomIdx;
                pushEntity(sceneFloor);

                for (auto &batchProp : sceneFloor.meshBatchProps)
                {
                    batchProp.textureName = pillarTypes[uint32(pillarTypes.size() * ud01(generator))];
                }
                sceneFloor.transform.setTranslation(offset + Vector3D(-450, -450, 250));
                sceneFloor.name = TCHAR("pillar4_") + roomIdx;
                pushEntity(sceneFloor);
            }
        }
    }
    // Special scene
    {
        PBRSceneEntity carsFloor;
        carsFloor.name = TCHAR("ShowroomFloor");
        carsFloor.meshAsset = cylinder;
        carsFloor.transform.setScale(Vector3D(13, 13, 1));
        carsFloor.transform.setTranslation(Vector3D(0, 2800, -45));
        for (uint32 batchIdx = 0; batchIdx < carsFloor.meshAsset->meshBatches.size(); ++batchIdx)
        {
            carsFloor.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties{ LinearColorConst::WHITE, 1.f, 1.f, floorTextureScale,
                                                                                   TCHAR("Tiles074"), &texturedPipelineContext });
        }
        pushEntity(carsFloor);

        PBRSceneEntity car;
        car.name = TCHAR("DodgeChallenger");
        car.meshAsset = static_cast<StaticMeshAsset *>(assetManager.getAsset(car.name));
        fatalAssertf(car.meshAsset, "Failed finding car mesh %s", car.name.getChar());
        car.transform.setTranslation(Vector3D(0, 2800, 0));
        for (uint32 batchIdx = 0; batchIdx < car.meshAsset->meshBatches.size(); ++batchIdx)
        {
            car.meshBatchProps.emplace_back(PBRSceneEntity::BatchProperties{ LinearColorConst::WHITE, 1.f, 1.f, Vector2D::ONE,
                                                                             car.name + car.meshAsset->meshBatches[batchIdx].name,
                                                                             &texturedPipelineContext });
        }
        pushEntity(car);

        // SpotLight heroLight;
        // heroLight.name = TCHAR("HeroLight");
        // heroLight.transform.setTranslation(car.transform.getTranslation() + Vector3D(0, 0, 400));
        // heroLight.transform.setRotation(Rotation(0, 90, 0));
        // heroLight.radius = 600;
        // heroLight.innerCone = 72;
        // heroLight.outerCone = 76;
        // heroLight.lightcolor = LinearColorConst::WHITE;
        // heroLight.lumen = 500;
        // pushSpt(heroLight);
    }

    sceneVolume.reinitialize(entities, Vector3D(50, 50, 50));
}

void ExperimentalEnginePBR::createSceneRenderData(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    uint32 totalVertexLen = 0;
    uint32 totalIdxLen = 0;

    for (const PBRSceneEntity &entity : sceneData)
    {
        if (meshVertIdxOffset.emplace(entity.meshAsset, std::pair<uint32, uint32>{}).second)
        {
            totalVertexLen += uint32(entity.meshAsset->getVertexBuffer()->getResourceSize());
            totalIdxLen += uint32(entity.meshAsset->getIndexBuffer()->getResourceSize());
        }
    }

    // Initialize scene vertex and index buffer
    sceneVertexBuffer
        = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(StaticMeshVertex), totalVertexLen / sizeof(StaticMeshVertex));
    sceneIndexBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, sizeof(uint32), totalIdxLen / sizeof(uint32));
    sceneVertexBuffer->init();
    sceneIndexBuffer->init();

    std::vector<BatchCopyBufferInfo> batchedCopies;
    uint32 vertOffset = 0;
    uint32 idxOffset = 0;
    for (auto &meshToVertIdx : meshVertIdxOffset)
    {
        meshToVertIdx.second = { vertOffset / sceneVertexBuffer->bufferStride(), idxOffset / sceneIndexBuffer->bufferStride() };

        BatchCopyBufferInfo &vertCopyInfo = batchedCopies.emplace_back();
        vertCopyInfo.dst = sceneVertexBuffer;
        vertCopyInfo.src = meshToVertIdx.first->getVertexBuffer();
        vertCopyInfo.copyInfo = CopyBufferInfo{ 0, vertOffset, uint32(meshToVertIdx.first->getVertexBuffer()->getResourceSize()) };

        BatchCopyBufferInfo &idxCopyInfo = batchedCopies.emplace_back();
        idxCopyInfo.dst = sceneIndexBuffer;
        idxCopyInfo.src = meshToVertIdx.first->getIndexBuffer();
        idxCopyInfo.copyInfo = CopyBufferInfo{ 0, idxOffset, uint32(meshToVertIdx.first->getIndexBuffer()->getResourceSize()) };

        vertOffset += uint32(meshToVertIdx.first->getVertexBuffer()->getResourceSize());
        idxOffset += uint32(meshToVertIdx.first->getIndexBuffer()->getResourceSize());
    }
    cmdList->copyBuffer(batchedCopies);
}

void ExperimentalEnginePBR::destroyScene()
{
    ENQUEUE_COMMAND(DestroyScene)
    (
        [this](class IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            sceneVertexBuffer.reset();
            sceneIndexBuffer.reset();
        }
    );
    sceneData.clear();
}

void ExperimentalEnginePBR::createShaderParameters(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    const PipelineBase *singleColPipeline = static_cast<const GraphicsPipelineBase *>(singleColorPipelineContext.getPipeline());
    const PipelineBase *texturedPipeline = static_cast<const GraphicsPipelineBase *>(texturedPipelineContext.getPipeline());
    // Since view data and other view related data are at set 0
    viewParameters = graphicsHelper->createShaderParameters(
        graphicsInstance, singleColPipeline->getParamLayoutAtSet(ShaderParameterUtility::VIEW_UNIQ_SET)
    );
    viewParameters->setResourceName(TCHAR("View"));
    // Bindless with all texture
    globalBindlessParameters = graphicsHelper->createShaderParameters(
        graphicsInstance, singleColPipeline->getParamLayoutAtSet(ShaderParameterUtility::BINDLESS_SET)
    );
    globalBindlessParameters->setResourceName(TCHAR("GlobalBindless"));
    // All vertex type's instance data(we have only static)
    instanceParameters = graphicsHelper->createShaderParameters(
        graphicsInstance, singleColPipeline->getParamLayoutAtSet(ShaderParameterUtility::INSTANCE_UNIQ_SET)
    );
    instanceParameters->setResourceName(TCHAR("StaticVertexInstances"));
    // All material parameters, we have single color and textured
    ShaderParametersRef singleColShaderParams = graphicsHelper->createShaderParameters(
        graphicsInstance, singleColPipeline->getParamLayoutAtSet(ShaderParameterUtility::SHADER_UNIQ_SET)
    );
    singleColShaderParams->setResourceName(TCHAR("SingleColorShaderParams"));
    ShaderParametersRef texturedShaderParams = graphicsHelper->createShaderParameters(
        graphicsInstance, texturedPipeline->getParamLayoutAtSet(ShaderParameterUtility::SHADER_UNIQ_SET)
    );
    texturedShaderParams->setResourceName(TCHAR("TexturedShaderParams"));
    sceneShaderUniqParams[&singleColorPipelineContext] = singleColShaderParams;
    sceneShaderUniqParams[&texturedPipelineContext] = texturedShaderParams;

    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    uint32 swapchainCount = windowCanvas->imagesCount();
    lightTextures.setNewSwapchain(windowCanvas);
    drawQuadTextureDescs.setNewSwapchain(windowCanvas);
    drawQuadNormalDescs.setNewSwapchain(windowCanvas);
    drawQuadAmbientDescs.setNewSwapchain(windowCanvas);
    drawQuadRoughDescs.setNewSwapchain(windowCanvas);
    drawQuadMetalDescs.setNewSwapchain(windowCanvas);
    drawQuadDepthDescs.setNewSwapchain(windowCanvas);
    drawLitColorsDescs.setNewSwapchain(windowCanvas);

    // Light related descriptors
    // as 2 and 3 are textures and light data
    const GraphicsResource *pbrModelNoShadowDescLayout = drawPbrNoShadowPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    const GraphicsResource *pbrModelWithShadowDescLayout = drawPbrWithShadowPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    lightCommon = graphicsHelper->createShaderParameters(graphicsInstance, pbrModelNoShadowDescLayout, { 2, 3 });
    lightCommon->setResourceName(TCHAR("LightCommon"));

    uint32 lightDataCount = uint32(Math::max(1u, Math::max(scenePointLights.size(), sceneSpotLights.size())));
    // -1 as we have 1 shadowed
    lightDataCount = uint32(Math::ceil(lightDataCount / float(ARRAY_LENGTH(PBRLightArray::spotLits)))) - 1;
    lightData.resize(lightDataCount);
    for (uint32 i = 0; i < lightDataCount; ++i)
    {
        // as 1 and 2 are light common and textures
        lightData[i] = graphicsHelper->createShaderParameters(graphicsInstance, pbrModelNoShadowDescLayout, { 1, 2 });
        lightData[i]->setResourceName(
            TCHAR("Light_") + String::toString(i * ARRAY_LENGTH(PBRLightArray::spotLits)) + TCHAR("to") + String::toString(i * ARRAY_LENGTH(PBRLightArray::spotLits) + ARRAY_LENGTH(PBRLightArray::spotLits)));
    }
    // as 1 and 2 are light common and textures
    lightDataShadowed = graphicsHelper->createShaderParameters(graphicsInstance, pbrModelWithShadowDescLayout, { 1, 2 });
    lightDataShadowed->setResourceName(TCHAR("ShadowedLights"));
    // Light shadow depth drawing related, Views from 4th descriptors set
    const GraphicsResource *drawLightDepth
        = directionalShadowPipelineContext.getPipeline()->getParamLayoutAtSet(ShaderParameterUtility::SHADER_VARIANT_UNIQ_SET);
    directionalViewParam = graphicsHelper->createShaderParameters(graphicsInstance, drawLightDepth);
    directionalViewParam->setResourceName(TCHAR("DirectionalLightViewParams"));

    drawLightDepth = pointShadowPipelineContext.getPipeline()->getParamLayoutAtSet(ShaderParameterUtility::SHADER_VARIANT_UNIQ_SET);
    for (uint32 i = 0; i < pointShadowRTs.size() && pointShadowRTs[i]; ++i)
    {
        pointViewParams[i] = graphicsHelper->createShaderParameters(graphicsInstance, drawLightDepth);
        pointViewParams[i]->setResourceName(TCHAR("PointDepthViewParams_") + String::toString(i));
    }
    // Since spot need no additional views so no 2nd set
    drawLightDepth = spotShadowPipelineContext.getPipeline()->getParamLayoutAtSet(ShaderParameterUtility::VIEW_UNIQ_SET);
    for (uint32 i = 0; i < spotShadowRTs.size() && spotShadowRTs[i]; ++i)
    {
        spotViewParams[i] = graphicsHelper->createShaderParameters(graphicsInstance, drawLightDepth);
        spotViewParams[i]->setResourceName(TCHAR("SpotDepthViewParams_") + String::toString(i));
    }

    const GraphicsResource *drawQuadDescLayout = resolveToPresentPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        const String iString = String::toString(i);
        lightTextures.set(graphicsHelper->createShaderParameters(graphicsInstance, pbrModelNoShadowDescLayout, { 1, 3 }), i);
        lightTextures.getResources()[i]->setResourceName(TCHAR("LightFrameCommon_") + iString);

        drawQuadTextureDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadTextureDescs.getResources()[i]->setResourceName(TCHAR("QuadUnlit_") + iString);
        drawQuadNormalDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadNormalDescs.getResources()[i]->setResourceName(TCHAR("QuadNormal_") + iString);
        drawQuadDepthDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadDepthDescs.getResources()[i]->setResourceName(TCHAR("QuadDepth_") + iString);
        drawQuadAmbientDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadAmbientDescs.getResources()[i]->setResourceName(TCHAR("QuadAmb_") + iString);
        drawQuadRoughDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadRoughDescs.getResources()[i]->setResourceName(TCHAR("QuadRough_") + iString);
        drawQuadMetalDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadMetalDescs.getResources()[i]->setResourceName(TCHAR("QuadMetal_") + iString);

        drawLitColorsDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawLitColorsDescs.getResources()[i]->setResourceName(TCHAR("QuadLit_") + iString);
    }

    clearInfoParams = graphicsHelper->createShaderParameters(graphicsInstance, clearQuadPipelineContext.getPipeline()->getParamLayoutAtSet(0));
    clearInfoParams->setResourceName(TCHAR("ClearInfo"));

    camViewAndInstanceParams
        = graphicsHelper->createShaderParameters(graphicsInstance, drawLinesDWritePipelineCntxt.getPipeline()->getParamLayoutAtSet(0));
    camViewAndInstanceParams->setResourceName(TCHAR("CameraGizmo"));

    camRTParams = graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout);
    camRTParams->setResourceName(TCHAR("CameraGizmoToScreenQuad"));

    setupShaderParameterParams(graphicsInstance, graphicsHelper);
}

void PointLight::update() const
{
    PbrPointLight ptLit;
    ptLit.ptLightColor_lumen = lightcolor;
    ptLit.ptLightColor_lumen.w() = lumen;
    ptLit.ptPos_radius = Vector4D(lightPos.x(), lightPos.y(), lightPos.z(), radius);
    (*paramCollection)->setBuffer(TCHAR("ptLits"), ptLit, index);

    if (shadowMap != nullptr && shadowViewParams /* && (*shadowViewParams).isValid()*/)
    {
        for (uint32 i = 0; i < views.size(); ++i)
        {
            Matrix4 w2Clip = views[i].projectionMatrix() * views[i].viewMatrix().inverse();
            (*shadowViewParams)->setMatrixParam(TCHAR("w2Clip"), w2Clip, i);
            (*shadowViewParams)->setVector4Param(TCHAR("lightPosFarPlane"), Vector4D(lightPos, radius));
        }
    }
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
    (*paramCollection)->setBuffer(TCHAR("spotLits"), spotLit, index);

    if (shadowMap != nullptr && shadowViewParams /* && (*shadowViewParams).isValid()*/)
    {
        ViewData viewData;
        viewData.projection = view.projectionMatrix();
        viewData.view = view.viewMatrix();
        viewData.invProjection = viewData.projection.inverse();
        viewData.invView = viewData.view.inverse();
        Matrix4 ndcToTextureSpace(
            Vector4D(0.5f, 0.0f, 0.0f, 0.0f), Vector4D(0.0f, 0.5f, 0.0f, 0.0f), Vector4D(0.0f, 0.0f, 1.0f, 0.0f),
            Vector4D(0.5f, 0.5f, 0.0f, 1.0f)
        );

        (*paramCollection)->setMatrixParam(TCHAR("sptLitsW2C"), ndcToTextureSpace * viewData.projection * viewData.invView, index);
        (*shadowViewParams)->setBuffer(RenderSceneBase::VIEW_PARAM_NAME, viewData);
    }
}

void DirectionalLight::update() const
{
    auto dirLit = PbrDirectionalLight{ Vector4D(lightcolor), direction.fwdVector() };
    dirLit.lightColor_lumen.w() = lumen;
    (*paramCollection)->setBuffer(TCHAR("dirLit"), dirLit);

    uint32 count = (*paramCollection)->getUintParam(TCHAR("count"));
    // clear 8-11 bits
    count &= ~0x00000F00;
    count |= ((0x0000000F & cascadeCount) << 8);
    (*paramCollection)->setIntParam(TCHAR("count"), count);

    if (cascadeShadowMaps != nullptr && shadowViewParams /* && (*shadowViewParams).isValid()*/)
    {
        (*shadowViewParams)->setIntParam(TCHAR("cascadeCount"), cascadeCount);
        for (uint32 i = 0; i < cascadeCount; ++i)
        {
            Matrix4 w2Clip = cascades[i].cascadeView.projectionMatrix() * cascades[i].cascadeView.viewMatrix().inverse();
            Matrix4 ndcToTextureSpace(
                Vector4D(0.5f, 0.0f, 0.0f, 0.0f), Vector4D(0.0f, 0.5f, 0.0f, 0.0f), Vector4D(0.0f, 0.0f, 1.0f, 0.0f),
                Vector4D(0.5f, 0.5f, 0.0f, 1.0f)
            );

            (*paramCollection)->setFloatParam(TCHAR("cascadeFarPlane"), cascades[i].frustumFarDistance, i);
            (*paramCollection)->setMatrixParam(TCHAR("dirLitCascadesW2C"), ndcToTextureSpace * w2Clip, i);
            (*shadowViewParams)->setMatrixParam(TCHAR("cascadeW2Clip"), w2Clip, i);
        }
    }
}

void DirectionalLight::normalizeCascadeCoverage()
{
    float total = 0;
    for (const CascadeData &cascade : cascades)
    {
        total += cascade.frustumFract;
    }

    for (CascadeData &cascade : cascades)
    {
        cascade.frustumFract /= total;
    }
}

void ExperimentalEnginePBR::setupShaderParameterParams(IGraphicsInstance *, const GraphicsHelperAPI *)
{
    // Setting up global bind less
    {
        std::vector<TextureAsset *> allTextures = assetManager.getAssetsOfType<EAssetType::Texture2D, TextureAsset>();
        for (uint32 i = 0; i < allTextures.size(); ++i)
        {
            globalBindlessParameters->setTextureParam(
                TCHAR("globalSampledTexs"), allTextures[i]->getTexture()->getTextureResource(), linearFiltering, i
            );
            tex2dToBindlessIdx[allTextures[i]->getTexture()->getTextureResource()] = i;
        }
        // Setup any non imported image resources here
        globalBindlessParameters->init();
    }

    ViewData viewData;
    viewData.view = camera.viewMatrix();
    viewData.invView = viewData.view.inverse();
    viewData.projection = camera.projectionMatrix();
    viewData.invProjection = viewData.projection.inverse();
    viewParameters->setBuffer(RenderSceneBase::VIEW_PARAM_NAME, viewData);
    viewParameters->init();

    // Setting values to instance params and material shader params happens along with global draw
    // command data buffer setup Dummy resize
    instanceParameters->resizeRuntimeBuffer(TCHAR("instancesWrapper"), 1);
    instanceParameters->init();

    for (auto &shaderUniqParams : sceneShaderUniqParams)
    {
        // Dummy resize
        shaderUniqParams.second->resizeRuntimeBuffer(TCHAR("materials"), 1);
        shaderUniqParams.second->init();
    }

    lightCommon->setBuffer(RenderSceneBase::VIEW_PARAM_NAME, viewData);
    lightCommon->init();

    // Directional light at last to do Linear -> SRGB and ambient lights
    dirLight.paramCollection = &lightDataShadowed;
    dirLight.shadowViewParams = &directionalViewParam;
    dirLight.cascadeShadowMaps = directionalShadowRT;
    dirLight.cascades.resize(dirLight.cascadeCount);
    dirLight.cascades[0].frustumFract = 0.1f;
    dirLight.cascades[1].frustumFract = 0.25f;
    dirLight.cascades[2].frustumFract = 0.30f;
    dirLight.cascades[3].frustumFract = 0.35f;
    dirLight.normalizeCascadeCoverage();
    lightDataShadowed->setFloatParam(TCHAR("gamma"), gamma);
    lightDataShadowed->setFloatParam(TCHAR("exposure"), exposure);
    lightDataShadowed->setTextureParam(TCHAR("directionalLightCascades"), dirLight.cascadeShadowMaps->getTextureResource(), shadowFiltering);
    for (uint32 i = 0; i < pointShadowRTs.size(); ++i)
    {
        ImageResourceRef texture = pointShadowRTs[i] ? pointShadowRTs[i]->getTextureResource() : GlobalBuffers::dummyCube();
        lightDataShadowed->setTextureParam(TCHAR("pointShadowMaps"), texture, shadowFiltering, i);
    }
    for (uint32 i = 0; i < spotShadowRTs.size(); ++i)
    {
        ImageResourceRef texture = spotShadowRTs[i] ? spotShadowRTs[i]->getTextureResource() : GlobalBuffers::dummyDepth();
        lightDataShadowed->setTextureParam(TCHAR("spotLightShadowMaps"), texture, shadowFiltering, i);
    }
    // count will be min up to 8
    uint32 shadowedCount = lightDataShadowed->getUintParam(TCHAR("count"));
    // clear 0-7 bits
    shadowedCount &= ~0x000000FF;
    shadowedCount |= (Math::min(sceneSpotLights.size(), 8) & 0x0000000F) | ((Math::min(scenePointLights.size(), 8) & 0x0000000F) << 4);
    lightDataShadowed->setIntParam(TCHAR("count"), shadowedCount);
    setupLightShaderData();
    lightDataShadowed->init();
    for (ShaderParametersRef &light : lightData)
    {
        light->init();
    }
    directionalViewParam->init();
    for (ShaderParametersRef &shadowView : pointViewParams)
    {
        if (shadowView.isValid())
        {
            shadowView->init();
        }
    }
    for (ShaderParametersRef &shadowView : spotViewParams)
    {
        if (shadowView.isValid())
        {
            shadowView->init();
        }
    }

    uint32 swapchainCount = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow())->imagesCount();
    ImageViewInfo ambImageViewInfo;
    ambImageViewInfo.componentMapping.g = ambImageViewInfo.componentMapping.b = ambImageViewInfo.componentMapping.a
        = ambImageViewInfo.componentMapping.r = EPixelComponentMapping::R;
    ImageViewInfo roughImageViewInfo;
    roughImageViewInfo.componentMapping.g = roughImageViewInfo.componentMapping.b = roughImageViewInfo.componentMapping.a
        = roughImageViewInfo.componentMapping.r = EPixelComponentMapping::G;
    ImageViewInfo metalImageViewInfo;
    metalImageViewInfo.componentMapping.g = metalImageViewInfo.componentMapping.b = metalImageViewInfo.componentMapping.a
        = metalImageViewInfo.componentMapping.r = EPixelComponentMapping::B;
    ImageViewInfo depthImageViewInfo;
    depthImageViewInfo.componentMapping.g = depthImageViewInfo.componentMapping.b = depthImageViewInfo.componentMapping.a
        = depthImageViewInfo.componentMapping.r = EPixelComponentMapping::R;
    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        GenericRenderPassProperties renderProps = GlobalBuffers::getFramebufferRenderpassProps(ERenderPassFormat::Multibuffer);
        std::vector<ImageResourceRef> multibufferRts = GBuffers::getGbufferAttachments(ERenderPassFormat::Multibuffer, i);
        const int32 fbIncrement = renderProps.bOneRtPerFormat ? 1 : 2;
        const int32 resolveIdxOffset = renderProps.bOneRtPerFormat ? 0 : 1;

        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssUnlitColor"), multibufferRts[(0 * fbIncrement) + resolveIdxOffset], nearestFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssNormal"), multibufferRts[(1 * fbIncrement) + resolveIdxOffset], nearestFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(TCHAR("ssARM"), multibufferRts[(2 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam(TCHAR("ssDepth"), multibufferRts[(3 * fbIncrement)], depthFiltering);
        lightTextures.getResources()[i]->setTextureParamViewInfo(TCHAR("ssDepth"), depthImageViewInfo);
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssColor"), frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(TCHAR("brdfLUT"), GlobalBuffers::integratedBrdfLUT(), nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("envMap"), envMaps[selectedEnv]->getEnvironmentMap()->getTextureResource(), linearFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("diffuseIrradMap"), envMaps[selectedEnv]->getDiffuseIrradianceMap()->getTextureResource(), linearFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("specEnvMap"), envMaps[selectedEnv]->getSpecularIrradianceMap()->getTextureResource(), linearFiltering
        );

        drawQuadTextureDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(0 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadNormalDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(1 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadAmbientDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(2 * fbIncrement)], linearFiltering);
        drawQuadRoughDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(2 * fbIncrement)], linearFiltering);
        drawQuadMetalDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(2 * fbIncrement)], linearFiltering);
        drawQuadAmbientDescs.getResources()[i]->setTextureParamViewInfo(TCHAR("quadTexture"), ambImageViewInfo);
        drawQuadRoughDescs.getResources()[i]->setTextureParamViewInfo(TCHAR("quadTexture"), roughImageViewInfo);
        drawQuadMetalDescs.getResources()[i]->setTextureParamViewInfo(TCHAR("quadTexture"), metalImageViewInfo);
        drawQuadDepthDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(3 * fbIncrement)], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParamViewInfo(TCHAR("quadTexture"), depthImageViewInfo);

        drawLitColorsDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), frameResources[i].lightingPassRt->getTextureResource(), linearFiltering
        );
    }
    lightTextures.init();
    drawQuadTextureDescs.init();
    drawQuadNormalDescs.init();
    drawQuadAmbientDescs.init();
    drawQuadRoughDescs.init();
    drawQuadMetalDescs.init();
    drawQuadDepthDescs.init();
    drawLitColorsDescs.init();

    clearInfoParams->setVector4Param(TCHAR("clearColor"), Vector4D(0, 0, 0, 0));
    clearInfoParams->init();

    Camera gizmoCamera;
    gizmoCamera.setClippingPlane(5.f, 305.f);
    gizmoCamera.setOrthoSize({ 290, 290 });
    gizmoCamera.cameraProjection = ECameraProjection::Orthographic;
    updateCamGizmoViewParams();
    camViewAndInstanceParams->setMatrixParam(TCHAR("projection"), gizmoCamera.projectionMatrix());
    camViewAndInstanceParams->resizeRuntimeBuffer(TCHAR("instancesWrapper"), 1);
    camViewAndInstanceParams->setMatrixParam(TCHAR("model"), Matrix4::IDENTITY);
    camViewAndInstanceParams->init();

    camRTParams->setTextureParam(TCHAR("quadTexture"), camGizmoColorTexture->getTextureResource(), linearFiltering);
    camRTParams->init();
}

void ExperimentalEnginePBR::reupdateTextureParamsOnResize()
{
    uint32 swapchainCount = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow())->imagesCount();

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        GenericRenderPassProperties renderProps = GlobalBuffers::getFramebufferRenderpassProps(ERenderPassFormat::Multibuffer);
        std::vector<ImageResourceRef> multibufferRts = GBuffers::getGbufferAttachments(ERenderPassFormat::Multibuffer, i);
        const int32 fbIncrement = renderProps.bOneRtPerFormat ? 1 : 2;
        const int32 resolveIdxOffset = renderProps.bOneRtPerFormat ? 0 : 1;

        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssUnlitColor"), multibufferRts[(0 * fbIncrement) + resolveIdxOffset], nearestFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssNormal"), multibufferRts[(1 * fbIncrement) + resolveIdxOffset], nearestFiltering
        );
        lightTextures.getResources()[i]->setTextureParam(TCHAR("ssARM"), multibufferRts[(2 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam(TCHAR("ssDepth"), multibufferRts[(3 * fbIncrement)], depthFiltering);
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssColor"), frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering
        );

        drawQuadTextureDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(0 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadNormalDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(1 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadAmbientDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(2 * fbIncrement)], linearFiltering);
        drawQuadRoughDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(2 * fbIncrement)], linearFiltering);
        drawQuadMetalDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(2 * fbIncrement)], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(3 * fbIncrement)], linearFiltering);
        drawLitColorsDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), frameResources[i].lightingPassRt->getTextureResource(), linearFiltering
        );
    }
}

void ExperimentalEnginePBR::reupdateEnvMap()
{
    ENQUEUE_COMMAND(WaitEnvMapUpdate)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            cmdList->flushAllcommands();
            const uint32 swapchainCount
                = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow())->imagesCount();
            for (uint32 i = 0; i < swapchainCount; ++i)
            {
                lightTextures.getResources()[i]->setTextureParam(
                    TCHAR("envMap"), envMaps[selectedEnv]->getEnvironmentMap()->getTextureResource(), linearFiltering
                );
                lightTextures.getResources()[i]->setTextureParam(
                    TCHAR("diffuseIrradMap"), envMaps[selectedEnv]->getDiffuseIrradianceMap()->getTextureResource(), linearFiltering
                );
                lightTextures.getResources()[i]->setTextureParam(
                    TCHAR("specEnvMap"), envMaps[selectedEnv]->getSpecularIrradianceMap()->getTextureResource(), linearFiltering
                );
            }
        }
    );
}

void ExperimentalEnginePBR::destroyShaderParameters()
{
    viewParameters.reset();
    globalBindlessParameters.reset();
    tex2dToBindlessIdx.clear();
    instanceParameters.reset();
    sceneShaderUniqParams.clear();

    lightCommon.reset();

    lightData.clear();
    lightDataShadowed.reset();
    for (ShaderParametersRef &ptShadowView : pointViewParams)
    {
        if (ptShadowView.isValid())
        {
            ptShadowView.reset();
        }
    }
    for (ShaderParametersRef &sptShadowView : spotViewParams)
    {
        if (sptShadowView.isValid())
        {
            sptShadowView.reset();
        }
    }
    directionalViewParam.reset();

    lightTextures.reset();
    drawQuadTextureDescs.reset();
    drawQuadNormalDescs.reset();
    drawQuadAmbientDescs.reset();
    drawQuadRoughDescs.reset();
    drawQuadMetalDescs.reset();
    drawQuadDepthDescs.reset();
    drawLitColorsDescs.reset();

    clearInfoParams.reset();

    camViewAndInstanceParams.reset();

    camRTParams.reset();
}

void ExperimentalEnginePBR::setupLightShaderData()
{
    lightDataShadowed->setIntParam(TCHAR("shadowFlags"), shadowFlags);

    setupLightShadowViews();

    std::vector<uint32> spotLightIdxs;
    std::vector<uint32> ptLightIdxs;
    sortSpotFromView(spotLightIdxs);
    sortPointsFromView(ptLightIdxs);

    dirLight.update();
    // Setup parameters to each lights
    for (int32 i = 0; i < 8; ++i)
    {
        if (i < spotLightIdxs.size())
        {
            sceneSpotLights[spotLightIdxs[i]].shadowViewParams = &spotViewParams[i];
            sceneSpotLights[spotLightIdxs[i]].shadowMap = spotShadowRTs[i];
            sceneSpotLights[spotLightIdxs[i]].drawCmdsBuffer = &spotDrawCmds[i];
            sceneSpotLights[spotLightIdxs[i]].paramCollection = &lightDataShadowed;
            sceneSpotLights[spotLightIdxs[i]].index = i;

            sceneSpotLights[spotLightIdxs[i]].update();
        }
        if (i < ptLightIdxs.size())
        {
            scenePointLights[ptLightIdxs[i]].shadowViewParams = &pointViewParams[i];
            scenePointLights[ptLightIdxs[i]].shadowMap = pointShadowRTs[i];
            scenePointLights[ptLightIdxs[i]].drawCmdsBuffer = &pointDrawCmds[i];
            scenePointLights[ptLightIdxs[i]].paramCollection = &lightDataShadowed;
            scenePointLights[ptLightIdxs[i]].index = i;

            scenePointLights[ptLightIdxs[i]].update();
        }
    }

    uint32 lightStartIdx = 8;
    for (ShaderParametersRef &light : lightData)
    {
        uint32 count = light->getUintParam(TCHAR("count"));
        // clear 0-7 bits
        count &= ~0x000000FF;

        uint32 rangeIdx = 0;
        for (; rangeIdx < ARRAY_LENGTH(PBRLightArray::spotLits) && (rangeIdx + lightStartIdx) < spotLightIdxs.size(); ++rangeIdx)
        {
            SpotLight &lightInfo = sceneSpotLights[spotLightIdxs[rangeIdx + lightStartIdx]];
            lightInfo.shadowMap = nullptr;
            lightInfo.shadowViewParams = nullptr;
            lightInfo.drawCmdsBuffer = nullptr;
            lightInfo.paramCollection = &light;
            lightInfo.index = rangeIdx;

            lightInfo.update();
        }
        count |= (0x0000000F & rangeIdx);
        rangeIdx = 0;
        for (; rangeIdx < ARRAY_LENGTH(PBRLightArray::ptLits) && (rangeIdx + lightStartIdx) < ptLightIdxs.size(); ++rangeIdx)
        {
            PointLight &lightInfo = scenePointLights[ptLightIdxs[rangeIdx + lightStartIdx]];
            lightInfo.shadowMap = nullptr;
            lightInfo.shadowViewParams = nullptr;
            lightInfo.drawCmdsBuffer = nullptr;
            lightInfo.paramCollection = &light;
            lightInfo.index = rangeIdx;

            lightInfo.update();
        }
        count |= ((0x0000000F & rangeIdx) << 4);

        light->setIntParam(TCHAR("count"), count);

        lightStartIdx += ARRAY_LENGTH(PBRLightArray::spotLits);
    }
}

void ExperimentalEnginePBR::resizeLightingRts(const Size2D &size)
{
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        frameResources[i].lightingPassRt->setTextureSize(size);
        frameResources[i].lightingPassResolved->setTextureSize(size);

        const IRenderTargetTexture *rtPtr = frameResources[i].lightingPassRt;
        rendererModule->getRenderManager()->clearExternInitRtsFramebuffer({ &rtPtr, 1 });
        rtPtr = frameResources[i].lightingPassResolved;
        rendererModule->getRenderManager()->clearExternInitRtsFramebuffer({ &rtPtr, 1 });

        // Used in debug rendering using depth map as read only target
        const IRenderTargetTexture *debugRenderingTexs[2]
            = { frameResources[i].lightingPassRt, GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, i)[3] };
        rendererModule->getRenderManager()->clearExternInitRtsFramebuffer(debugRenderingTexs);
    }
}

void ExperimentalEnginePBR::createFrameResources(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());

    RenderTextureCreateParams rtCreateParams;
    rtCreateParams.bSameReadWriteTexture = true;
    rtCreateParams.filtering = ESamplerFiltering::Linear;
    rtCreateParams.format = ERenderTargetFormat::RT_U8;
    rtCreateParams.sampleCount = EPixelSampleCount::SampleCount1;
    rtCreateParams.textureSize = ApplicationSettings::screenSize.get();

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        String name{ TCHAR("Frame") };
        name.append(String::toString(i));

        frameResources[i].usageWaitSemaphore.push_back(graphicsHelper->createSemaphore(graphicsInstance, (name + TCHAR("QueueSubmit")).c_str())
        );
        frameResources[i].usageWaitSemaphore.back()->init();
        frameResources[i].recordingFence = graphicsHelper->createFence(graphicsInstance, (name + TCHAR("RecordingGaurd")).c_str());
        frameResources[i].recordingFence->init();

        rtCreateParams.textureName = TCHAR("LightingRT_") + String::toString(i);
        frameResources[i].lightingPassRt = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
        rtCreateParams.textureName = TCHAR("LightingResolved_") + String::toString(i);
        frameResources[i].lightingPassResolved = TextureBase::createTexture<RenderTargetTexture>(rtCreateParams);
    }
}

void ExperimentalEnginePBR::destroyFrameResources()
{
    for (int32 i = 0; i < frameResources.size(); ++i)
    {
        frameResources[i].usageWaitSemaphore[0].reset();
        frameResources[i].recordingFence.reset();

        const IRenderTargetTexture *rtPtr = frameResources[i].lightingPassRt;
        rendererModule->getRenderManager()->clearExternInitRtsFramebuffer({ &rtPtr, 1 });
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassRt);
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassResolved);
    }
}

void ExperimentalEnginePBR::getPipelineContextForSubpass()
{
    auto multibufferRts = GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, 0);
    singleColorPipelineContext.forVertexType = EVertexType::StaticMesh;
    singleColorPipelineContext.materialName = TCHAR("SingleColor");
    singleColorPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffer;
    singleColorPipelineContext.swapchainIdx = 0;
    rendererModule->getRenderManager()->preparePipelineContext(&singleColorPipelineContext, multibufferRts);

    texturedPipelineContext.forVertexType = EVertexType::StaticMesh;
    texturedPipelineContext.materialName = TCHAR("Textured");
    texturedPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffer;
    texturedPipelineContext.swapchainIdx = 0;
    rendererModule->getRenderManager()->preparePipelineContext(&texturedPipelineContext, multibufferRts);

    fatalAssertf(
        GlobalRenderVariables::ENABLE_GEOMETRY_SHADERS, TCHAR("Geometry shader feature not supported in this device, so cannot use shadows")
        );
    spotShadowPipelineContext.forVertexType = pointShadowPipelineContext.forVertexType = directionalShadowPipelineContext.forVertexType
        = EVertexType::StaticMesh;
    spotShadowPipelineContext.materialName = pointShadowPipelineContext.materialName = directionalShadowPipelineContext.materialName
        = TCHAR("Default");
    spotShadowPipelineContext.swapchainIdx = pointShadowPipelineContext.swapchainIdx = directionalShadowPipelineContext.swapchainIdx = 0;
    spotShadowPipelineContext.renderpassFormat = ERenderPassFormat::Depth;
    pointShadowPipelineContext.renderpassFormat = ERenderPassFormat::PointLightDepth;

    directionalShadowPipelineContext.renderpassFormat = ERenderPassFormat::DirectionalLightDepth;
    const IRenderTargetTexture *rtPtr = directionalShadowRT;
    rendererModule->getRenderManager()->preparePipelineContext(&directionalShadowPipelineContext, { &rtPtr, 1 });
    if (spotShadowRTs[0])
    {
        rtPtr = spotShadowRTs[0];
        rendererModule->getRenderManager()->preparePipelineContext(&spotShadowPipelineContext, { &rtPtr, 1 });
    }
    if (pointShadowRTs[0])
    {
        rtPtr = pointShadowRTs[0];
        rendererModule->getRenderManager()->preparePipelineContext(&pointShadowPipelineContext, { &rtPtr, 1 });
    }

    const IRenderTargetTexture *lightingPassRts[] = { frameResources[0].lightingPassRt };
    const IRenderTargetTexture *lightingPassResolveRts[] = { frameResources[0].lightingPassResolved };
    // Using depth map as read only target
    const IRenderTargetTexture *lightPassAndDepthRts[]
        = { frameResources[0].lightingPassRt, GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, 0)[3] };
    // PBR model
    drawPbrWithShadowPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawPbrWithShadowPipelineContext.materialName = TCHAR("PBRLightsWithShadow");
    rendererModule->getRenderManager()->preparePipelineContext(&drawPbrWithShadowPipelineContext, lightingPassRts);
    drawPbrNoShadowPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawPbrNoShadowPipelineContext.materialName = TCHAR("PBRLightsNoShadow");
    rendererModule->getRenderManager()->preparePipelineContext(&drawPbrNoShadowPipelineContext, lightingPassRts);

    sceneDebugLinesPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    sceneDebugLinesPipelineContext.materialName = TCHAR("Draw3DColoredPerVertexLine");
    // Using depth map as read only target
    rendererModule->getRenderManager()->preparePipelineContext(&sceneDebugLinesPipelineContext, lightPassAndDepthRts);

    drawLinesDWritePipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
    drawLinesDWritePipelineCntxt.materialName = TCHAR("Draw3DColoredPerVertexLineDWrite");
    const IRenderTargetTexture *gizmoRts[] = { camGizmoColorTexture, camGizmoDepthTarget };
    rendererModule->getRenderManager()->preparePipelineContext(&drawLinesDWritePipelineCntxt, gizmoRts);

    drawGridDTestPipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
    drawGridDTestPipelineCntxt.materialName = TCHAR("DrawGridDTest");
    rendererModule->getRenderManager()->preparePipelineContext(&drawGridDTestPipelineCntxt, lightPassAndDepthRts);

    clearQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    clearQuadPipelineContext.materialName = TCHAR("ClearRT");
    rendererModule->getRenderManager()->preparePipelineContext(&clearQuadPipelineContext, lightingPassResolveRts);

    resolveLightRtPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    resolveLightRtPipelineContext.materialName = TCHAR("DrawQuadFromTexture");
    rendererModule->getRenderManager()->preparePipelineContext(&resolveLightRtPipelineContext, lightingPassResolveRts);

    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    resolveToPresentPipelineContext.windowCanvas = windowCanvas;
    resolveToPresentPipelineContext.materialName = TCHAR("DrawQuadFromTexture");
    resolveToPresentPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    resolveToPresentPipelineContext.swapchainIdx = 0;
    rendererModule->getRenderManager()->preparePipelineContext(&resolveToPresentPipelineContext);

    overBlendedQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    overBlendedQuadPipelineContext.materialName = TCHAR("DrawOverBlendedQuadFromTexture");
    rendererModule->getRenderManager()->preparePipelineContext(&overBlendedQuadPipelineContext, lightingPassRts);
}

void ExperimentalEnginePBR::clearPipelineContexts()
{
    singleColorPipelineContext.reset();
    texturedPipelineContext.reset();
    spotShadowPipelineContext.reset();
    pointShadowPipelineContext.reset();
    directionalShadowPipelineContext.reset();
    drawPbrWithShadowPipelineContext.reset();
    drawPbrNoShadowPipelineContext.reset();
    sceneDebugLinesPipelineContext.reset();
    drawLinesDWritePipelineCntxt.reset();
    drawGridDTestPipelineCntxt.reset();
    clearQuadPipelineContext.reset();
    resolveLightRtPipelineContext.reset();
    resolveToPresentPipelineContext.reset();
    overBlendedQuadPipelineContext.reset();
}

void ExperimentalEnginePBR::createPipelineResources(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    clearValues.colors.resize(singleColorPipelineContext.getFb()->textures.size(), LinearColorConst::BLACK);

    // Shader pipeline's buffers and image access
    createShaderParameters(graphicsInstance, graphicsHelper);
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
    if (application->inputSystem->isKeyPressed(Keys::RMB))
    {
        cameraRotation.yaw()
            += application->inputSystem->analogState(AnalogStates::RelMouseX)->currentValue * timeData.activeTimeDilation * 0.25f;
        cameraRotation.pitch()
            += application->inputSystem->analogState(AnalogStates::RelMouseY)->currentValue * timeData.activeTimeDilation * 0.25f;
        bCamRotated = true;
    }
    float camSpeedModifier = 1;
    if (application->inputSystem->isKeyPressed(Keys::LSHIFT))
    {
        camSpeedModifier = 2;
    }
    if (application->inputSystem->isKeyPressed(Keys::A))
    {
        cameraTranslation -= cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::D))
    {
        cameraTranslation += cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::W))
    {
        cameraTranslation += cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::S))
    {
        cameraTranslation -= cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::Q))
    {
        cameraTranslation -= Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::E))
    {
        cameraTranslation += Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = RotationMatrix::fromZX(Vector3D::UP, cameraRotation.fwdVector()).asRotation();
        bCamRotated = true;
    }

    auto projectionChanged = [this]()
    {
        camera.cameraProjection = projection;
        camera.setFOV((110.f * ApplicationSettings::surfaceSize.get().x) / (ApplicationSettings::surfaceSize.get().y * 1.78f), 90.f);

        Matrix4 projectionMat = camera.projectionMatrix();
        Matrix4 invProjectionMat = projectionMat.inverse();
        viewParameters->setMatrixParam(TCHAR("projection"), projectionMat);
        viewParameters->setMatrixParam(TCHAR("invProjection"), invProjectionMat);
        lightCommon->setMatrixParam(TCHAR("projection"), projectionMat);
        lightCommon->setMatrixParam(TCHAR("invProjection"), invProjectionMat);
    };
    if (camera.cameraProjection != projection)
    {
        projectionChanged();
    }
    static DelegateHandle handle = ApplicationSettings::surfaceSize.onConfigChanged().bindLambda(
        [projectionChanged](Size2D, Size2D)
        {
            projectionChanged();
        }
    );

    camera.setRotation(cameraRotation);
    camera.setTranslation(cameraTranslation);

    viewDataTemp.view = camera.viewMatrix();
    viewDataTemp.invView = viewDataTemp.view.inverse();
    viewParameters->setMatrixParam(TCHAR("view"), viewDataTemp.view);
    viewParameters->setMatrixParam(TCHAR("invView"), viewDataTemp.invView);
    lightCommon->setMatrixParam(TCHAR("view"), viewDataTemp.view);
    lightCommon->setMatrixParam(TCHAR("invView"), viewDataTemp.invView);

    if (bCamRotated)
    {
        updateCamGizmoViewParams();
        ENQUEUE_COMMAND(CameraGizmoUpdate)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *)
            {
                updateCamGizmoCapture(cmdList, graphicsInstance);
            }
        );
    }
}

void ExperimentalEnginePBR::onStartUp()
{
    TestGameEngine::onStartUp();

    camera.cameraProjection = projection;
    camera.setOrthoSize({ 1280, 720 });
    camera.setClippingPlane(0.1f, 6000.f);
    camera.setFOV((110.f * ApplicationSettings::surfaceSize.get().x) / (ApplicationSettings::surfaceSize.get().y * 1.78f), 90.f);

    cameraTranslation = Vector3D(0.f, 1.f, 0.0f).safeNormalized() * (500);
    cameraTranslation.z() += 200;

    camera.setTranslation(cameraTranslation);
    camera.lookAt(Vector3D::ZERO);
    cameraRotation = camera.rotation();

    thinColor = LinearColorConst::GRAY;
    thickColor = LinearColorConst::WHITE;

    getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(shared_from_this()));
    createScene();

    textures = assetManager.getAssetsOfType<EAssetType::Texture2D, TextureAsset>();
    std::sort(textures.begin(), textures.end(), SortAssetByName<true>());
    textureNames.resize(textures.size() + 1);
    textureNameStrs.reserve(textures.size() + 1);
    textureNameStrs.emplace_back(TCHAR_TO_UTF8(noneString.getChar()));
    for (const TextureAsset *texture : textures)
    {
        textureNameStrs.emplace_back(TCHAR_TO_UTF8(texture->assetName().getChar()));
    }
    for (uint32 i = 0; i < textureNameStrs.size(); ++i)
    {
        textureNames[i] = textureNameStrs[i].c_str();
    }
    selectedTexture = 0;

    envMaps = assetManager.getAssetsOfType<EAssetType::CubeMap, EnvironmentMapAsset>();
    std::sort(envMaps.begin(), envMaps.end(), SortAssetByName<true>());
    envMapNames.resize(envMaps.size());
    envMapNameStrs.reserve(envMaps.size());
    for (const EnvironmentMapAsset *envMap : envMaps)
    {
        envMapNameStrs.emplace_back(TCHAR_TO_UTF8(envMap->assetName().getChar()));
    }
    for (uint32 i = 0; i < envMapNameStrs.size(); ++i)
    {
        envMapNames[i] = envMapNameStrs[i].c_str();
    }
    selectedEnv = 0;

    tempTest();

    ENQUEUE_COMMAND(RenderStartup)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            createSceneRenderData(cmdList, graphicsInstance, graphicsHelper);
            startUpRenderInit(graphicsInstance, graphicsHelper);
            updateCamGizmoCapture(cmdList, graphicsInstance);
        }
    );
}

void ExperimentalEnginePBR::startUpRenderInit(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    int32 swapchainCount = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow())->imagesCount();
    frameResources.resize(swapchainCount);
    GBuffers::initialize(swapchainCount);

    createFrameResources(graphicsInstance, graphicsHelper);
    createImages(graphicsInstance, graphicsHelper);
    getPipelineContextForSubpass();
    createPipelineResources(graphicsInstance, graphicsHelper);
    createDrawCmdsBuffer(graphicsInstance, graphicsHelper);
}

void ExperimentalEnginePBR::onQuit()
{
    ENQUEUE_COMMAND(EngineQuit)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            cmdList->flushAllcommands();
            renderQuit();
        }
    );

    getImGuiManager().removeLayer(std::static_pointer_cast<IImGuiLayer>(shared_from_this()));

    tempTestQuit();
    TestGameEngine::onQuit();
}

void ExperimentalEnginePBR::renderQuit()
{
    GBuffers::destroy();
    clearPipelineContexts();

    destroyDrawCmdsBuffer();
    destroyPipelineResources();
    destroyImages();
    destroyFrameResources();

    destroyScene();
}

void ExperimentalEnginePBR::frameRender(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    SemaphoreRef waitSemaphore;
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    uint32 index = windowCanvas->requestNextImage(&waitSemaphore, nullptr);

    resolveToPresentPipelineContext.swapchainIdx = index;
    resolveToPresentPipelineContext.windowCanvas = windowCanvas;
    rendererModule->getRenderManager()->preparePipelineContext(&resolveToPresentPipelineContext);

    auto gbufferRts = GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, index);
    rendererModule->getRenderManager()->preparePipelineContext(&singleColorPipelineContext, gbufferRts);

    const IRenderTargetTexture *lightRtAttachments[] = { frameResources[index].lightingPassRt };
    const IRenderTargetTexture *lightResolvedAttachments[] = { frameResources[index].lightingPassResolved };
    rendererModule->getRenderManager()->preparePipelineContext(&drawPbrWithShadowPipelineContext, lightRtAttachments);
    rendererModule->getRenderManager()->preparePipelineContext(&drawPbrNoShadowPipelineContext, lightRtAttachments);
    rendererModule->getRenderManager()->preparePipelineContext(&resolveLightRtPipelineContext, lightResolvedAttachments);

    GraphicsPipelineQueryParams queryParam;
    queryParam.cullingMode = ECullingMode::BackFace;
    queryParam.drawMode = EPolygonDrawMode::Fill;

    // finishCmd will wait
    // if (!frameResources[index].recordingFence->isSignaled())
    //{
    //    frameResources[index].recordingFence->waitForSignal();
    //}
    // frameResources[index].recordingFence->resetSignal();

    QuantizedBox2D viewport;
    // Since view matrix positive y is along up while vulkan positive y in view is down
    viewport.minBound.x = 0;
    viewport.minBound.y = ApplicationSettings::screenSize.get().y;
    viewport.maxBound.x = ApplicationSettings::screenSize.get().x;
    viewport.maxBound.y = 0;

    QuantizedBox2D scissor;
    scissor.minBound = { 0, 0 };
    scissor.maxBound = ApplicationSettings::screenSize.get();

    String cmdName = TCHAR("FrameRender") + String::toString(index);
    cmdList->finishCmd(cmdName);

    const GraphicsResource *cmdBuffer = cmdList->startCmd(cmdName, EQueueFunction::Graphics, true);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ExperimentalEnginePBRFrame);

        renderShadows(cmdList, graphicsInstance, cmdBuffer);

        cmdList->cmdBeginRenderPass(cmdBuffer, singleColorPipelineContext, scissor, {}, clearValues);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, MainUnlitPass);

            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, sceneVertexBuffer, 0);
            cmdList->cmdBindIndexBuffer(cmdBuffer, sceneIndexBuffer);

            // Bind less
            cmdList->cmdBindDescriptorsSets(cmdBuffer, texturedPipelineContext, globalBindlessParameters);
            for (const auto &pipelineToOffsetCount : pipelineToDrawCmdOffsetCount)
            {
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, *pipelineToOffsetCount.first, { queryParam });

                ShaderParametersRef descSets[] = { viewParameters, instanceParameters, sceneShaderUniqParams[pipelineToOffsetCount.first] };
                // Shader material params set
                cmdList->cmdBindDescriptorsSets(cmdBuffer, *pipelineToOffsetCount.first, descSets);

                cmdList->cmdDrawIndexedIndirect(
                    cmdBuffer, allEntityDrawCmds, pipelineToOffsetCount.second.first, pipelineToOffsetCount.second.second,
                    allEntityDrawCmds->bufferStride()
                );
            }
        }
        cmdList->cmdEndRenderPass(cmdBuffer);

        // Drawing lighting quads
        viewport.minBound = Int2D(0, 0);
        viewport.maxBound = ApplicationSettings::screenSize.get();

        cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        if (frameVisualizeId == 0)
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, LightingPass);

            cmdList->cmdBeginRenderPass(cmdBuffer, resolveLightRtPipelineContext, scissor, {}, clearValues);
            {
                SCOPED_CMD_MARKER(cmdList, cmdBuffer, ClearLightingRTs);

                // Clear resolve first
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, clearQuadPipelineContext, { queryParam });
                cmdList->cmdBindDescriptorsSets(cmdBuffer, clearQuadPipelineContext, clearInfoParams);
                cmdList->cmdDrawVertices(cmdBuffer, 0, 3);
            }
            cmdList->cmdEndRenderPass(cmdBuffer);

            for (const ShaderParametersRef &light : lightData)
            {
                cmdList->cmdBeginRenderPass(cmdBuffer, drawPbrNoShadowPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawLight);
                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawPbrNoShadowPipelineContext, { queryParam });

                    ShaderParametersRef descSets[] = { lightCommon, *lightTextures, light };
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, drawPbrNoShadowPipelineContext, descSets);
                    cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
                }
                cmdList->cmdEndRenderPass(cmdBuffer);
                // Resolve drawn lights
                cmdList->cmdBeginRenderPass(cmdBuffer, resolveLightRtPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveLightRT);

                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, resolveLightRtPipelineContext, { queryParam });
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, resolveLightRtPipelineContext, *drawLitColorsDescs);

                    cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
                }
                cmdList->cmdEndRenderPass(cmdBuffer);
            }
            // Light with shadows, Final pass
            cmdList->cmdBeginRenderPass(cmdBuffer, drawPbrWithShadowPipelineContext, scissor, {}, clearValues);
            {
                SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawLightWithShadow);

                std::pair<String, std::any> pushConsts[] = {
                    {TCHAR("debugDrawFlags"), uint32(renderFlags)}
                };
                cmdList->cmdPushConstants(cmdBuffer, drawPbrWithShadowPipelineContext, pushConsts);
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawPbrWithShadowPipelineContext, { queryParam });

                ShaderParametersRef descSets[] = { lightCommon, *lightTextures, lightDataShadowed };
                cmdList->cmdBindDescriptorsSets(cmdBuffer, drawPbrWithShadowPipelineContext, descSets);
                cmdList->cmdDrawIndexed(cmdBuffer, 0, 3);
            }
            cmdList->cmdEndRenderPass(cmdBuffer);
        }
        else
        {
            ShaderParametersRef drawQuadDescs;
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

            if (drawQuadDescs.isValid())
            {
                rendererModule->getRenderManager()->preparePipelineContext(&resolveLightRtPipelineContext, lightRtAttachments);

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
        ImGuiDrawingContext drawingContext;
        drawingContext.cmdBuffer = cmdBuffer;
        drawingContext.rtTexture = lightRtAttachments[0];
        getImGuiManager().draw(cmdList, graphicsInstance, graphicsHelper, drawingContext);

        // Drawing final resolve to presenting surface quad
        viewport.minBound = Int2D(0, 0);
        viewport.maxBound = scissor.maxBound = ApplicationSettings::surfaceSize.get();

        cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        RenderPassAdditionalProps renderPassAdditionalProps;
        renderPassAdditionalProps.bUsedAsPresentSource = true;
        cmdList->cmdBeginRenderPass(cmdBuffer, resolveToPresentPipelineContext, scissor, renderPassAdditionalProps, clearValues);

        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveToSwapchain);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, resolveToPresentPipelineContext, { queryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, resolveToPresentPipelineContext, *drawLitColorsDescs);
            cmdList->cmdDrawVertices(cmdBuffer, 0, 3);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.waitOn = {
        {waitSemaphore, INDEX_TO_FLAG_MASK(EPipelineStages::FragmentShaderStage)}
    };
    submitInfo.signalSemaphores = {
        {frameResources[index].usageWaitSemaphore[0], INDEX_TO_FLAG_MASK(EPipelineStages::ColorAttachmentOutput)}
    };
    submitInfo.cmdBuffers = { cmdBuffer };

    cmdList->submitCmd(EQueuePriority::High, submitInfo, frameResources[index].recordingFence);

    // Presenting manually here as for Experimental we are not adding any widget to app main window and it gets skipped in presenting all drawn
    // windows
    cmdList->presentImage({ &windowCanvas, 1 }, { &index, 1 }, { frameResources[index].usageWaitSemaphore });
}

void ExperimentalEnginePBR::updateCamGizmoViewParams()
{
    Camera gizmoCam;
    gizmoCam.setTranslation(-camera.rotation().fwdVector() * 150);
    gizmoCam.lookAt(Vector3D::ZERO);

    camViewAndInstanceParams->setMatrixParam(TCHAR("invView"), gizmoCam.viewMatrix().inverse());
}

void ExperimentalEnginePBR::updateCamGizmoCapture(class IRenderCommandList *cmdList, IGraphicsInstance *)
{
    String cmdName = TCHAR("UpdateCameraGizmo");
    cmdList->finishCmd(cmdName);

    const GraphicsResource *cmdBuffer = cmdList->getCmdBuffer(cmdName);
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

            std::pair<String, std::any> pushConsts[] = {
                {TCHAR("ptSize"), 1.0f}
            };
            cmdList->cmdPushConstants(cmdBuffer, sceneDebugLinesPipelineContext, pushConsts);
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawLinesDWritePipelineCntxt, camViewAndInstanceParams);
            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getLineGizmoVertexIndexBuffers().first, 0);
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

void ExperimentalEnginePBR::renderShadows(class IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsResource *cmdBuffer)
{
    GraphicsPipelineQueryParams faceFillQueryParam;
    // Since we are drawing inverted backfaces are front face and vice versa for spot and directional
    // lights
    faceFillQueryParam.cullingMode = BIT_SET(shadowFlags, PBRShadowFlags::DrawingBackface) ? ECullingMode::BackFace : ECullingMode::FrontFace;
    faceFillQueryParam.drawMode = EPolygonDrawMode::Fill;

    // This will render shadows inverted y(1, -1) but we are fine with that
    QuantizedBox2D viewport(Int2D(0, 0), Int2D(directionalShadowRT->getTextureSize()));
    QuantizedBox2D scissor = viewport;

    SCOPED_CMD_MARKER(cmdList, cmdBuffer, RenderShadows);
    cmdList->cmdBindVertexBuffer(cmdBuffer, 0, sceneVertexBuffer, 0);
    cmdList->cmdBindIndexBuffer(cmdBuffer, sceneIndexBuffer);
    // Draw cascade first
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, DirectionalShadowCascade);
        cmdList->cmdBeginRenderPass(cmdBuffer, directionalShadowPipelineContext, scissor, {}, clearValues);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        // Bind and draw
        cmdList->cmdBindGraphicsPipeline(cmdBuffer, directionalShadowPipelineContext, { faceFillQueryParam });
        ShaderParametersRef descSets[] = { viewParameters, directionalViewParam, instanceParameters };
        cmdList->cmdBindDescriptorsSets(cmdBuffer, directionalShadowPipelineContext, descSets);
        cmdList->cmdDrawIndexedIndirect(cmdBuffer, allEntityDrawCmds, 0, allEntityDrawCmds->bufferCount(), allEntityDrawCmds->bufferStride());

        cmdList->cmdEndRenderPass(cmdBuffer);
    }

    // Draw spot lights
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, SpotLightShadows);
        for (const SpotLight &sptlit : sceneSpotLights)
        {
            if (sptlit.shadowViewParams && sptlit.shadowMap && sptlit.drawCmdsBuffer)
            {
                viewport = { Int2D(0, 0), Int2D(sptlit.shadowMap->getTextureSize()) };
                scissor = viewport;

                const IRenderTargetTexture *shadowMapRt = sptlit.shadowMap;
                rendererModule->getRenderManager()->preparePipelineContext(&spotShadowPipelineContext, { &shadowMapRt, 1 });

                cmdList->cmdBeginRenderPass(cmdBuffer, spotShadowPipelineContext, scissor, {}, clearValues);
                cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

                // Bind and draw
                // Since we are drawing inverted backfaces are front face and vice versa for spot
                // and directional lights
                faceFillQueryParam.cullingMode
                    = BIT_SET(shadowFlags, PBRShadowFlags::DrawingBackface) ? ECullingMode::BackFace : ECullingMode::FrontFace;
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, spotShadowPipelineContext, { faceFillQueryParam });
                cmdList->cmdBindDescriptorsSets(cmdBuffer, spotShadowPipelineContext, *sptlit.shadowViewParams);
#if SHADOWS_USE_CULLED_DRAW_CMDS
                cmdList->cmdDrawIndexedIndirect(
                    cmdBuffer, (*sptlit.drawCmdsBuffer), 0, sptlit.drawCmdCount, (*sptlit.drawCmdsBuffer)->bufferStride()
                );
#else
                cmdList->cmdDrawIndexedIndirect(
                    cmdBuffer, allEntityDrawCmds, 0, allEntityDrawCmds->bufferCount(), allEntityDrawCmds->bufferStride()
                );
#endif

                cmdList->cmdEndRenderPass(cmdBuffer);
            }
        }
    }

    // Draw point lights
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, PointLightShadows);
        for (const PointLight &ptlit : scenePointLights)
        {
            if (ptlit.shadowViewParams && ptlit.shadowMap && ptlit.drawCmdsBuffer)
            {
                viewport = { Int2D(0, ptlit.shadowMap->getTextureSize().y), Int2D(ptlit.shadowMap->getTextureSize().x, 0) };
                scissor = { Int2D(0, 0), Int2D(ptlit.shadowMap->getTextureSize()) };

                const IRenderTargetTexture *shadowMapRt = ptlit.shadowMap;
                rendererModule->getRenderManager()->preparePipelineContext(&pointShadowPipelineContext, { &shadowMapRt, 1 });

                cmdList->cmdBeginRenderPass(cmdBuffer, pointShadowPipelineContext, scissor, {}, clearValues);
                cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

                // Bind and draw
                faceFillQueryParam.cullingMode
                    = BIT_SET(shadowFlags, PBRShadowFlags::DrawingBackface) ? ECullingMode::FrontFace : ECullingMode::BackFace;
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, pointShadowPipelineContext, { faceFillQueryParam });
                ShaderParametersRef descSets[] = { *ptlit.shadowViewParams, instanceParameters };
                cmdList->cmdBindDescriptorsSets(cmdBuffer, pointShadowPipelineContext, descSets);
#if SHADOWS_USE_CULLED_DRAW_CMDS
                cmdList->cmdDrawIndexedIndirect(
                    cmdBuffer, *ptlit.drawCmdsBuffer, 0, ptlit.drawCmdCount, (*ptlit.drawCmdsBuffer)->bufferStride()
                );
#else
                cmdList->cmdDrawIndexedIndirect(
                    cmdBuffer, allEntityDrawCmds, 0, allEntityDrawCmds->bufferCount(), allEntityDrawCmds->bufferStride()
                );
#endif

                cmdList->cmdEndRenderPass(cmdBuffer);
            }
        }
    }
}

void ExperimentalEnginePBR::debugFrameRender(
    class IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsResource *cmdBuffer, uint32 swapchainIdx
)
{
    GraphicsPipelineQueryParams backfaceFillQueryParam;
    backfaceFillQueryParam.cullingMode = ECullingMode::BackFace;
    backfaceFillQueryParam.drawMode = EPolygonDrawMode::Fill;

    RenderPassAdditionalProps debugSceneDrawAdditionalProps;
    debugSceneDrawAdditionalProps.depthLoadOp = EAttachmentOp::LoadOp::Load;
    debugSceneDrawAdditionalProps.depthStoreOp = EAttachmentOp::StoreOp::DontCare;
    debugSceneDrawAdditionalProps.colorAttachmentLoadOp = EAttachmentOp::LoadOp::Load;

    // Drawing in scene first
    QuantizedBox2D viewport;
    // Since view matrix positive y is along up while vulkan positive y in view is down
    viewport.minBound.x = 0;
    viewport.minBound.y = ApplicationSettings::screenSize.get().y;
    viewport.maxBound.x = ApplicationSettings::screenSize.get().x;
    viewport.maxBound.y = 0;

    QuantizedBox2D scissor;
    scissor.minBound = { 0, 0 };
    scissor.maxBound = ApplicationSettings::screenSize.get();

    const IRenderTargetTexture *backFramebufferRts[]
        = { frameResources[swapchainIdx].lightingPassRt, GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, swapchainIdx)[3] };
#if DEV_BUILD
    rendererModule->getRenderManager()->preparePipelineContext(&sceneDebugLinesPipelineContext, backFramebufferRts);

    if (bDrawTbn && selection.type == GridEntity::Entity)
    {
        PBRSceneEntity &sceneEntity = sceneData[selection.idx];
        // Resetting viewport as we use mvp again
        viewport.minBound.x = 0;
        viewport.minBound.y = ApplicationSettings::screenSize.get().y;
        viewport.maxBound.x = ApplicationSettings::screenSize.get().x;
        viewport.maxBound.y = 0;

        SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawTBN);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        cmdList->cmdBeginRenderPass(cmdBuffer, sceneDebugLinesPipelineContext, scissor, debugSceneDrawAdditionalProps, clearValues);
        {
            GraphicsPipelineState pipelineState;
            pipelineState.pipelineQuery = backfaceFillQueryParam;
            pipelineState.lineWidth = 1.0f;
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, sceneDebugLinesPipelineContext, pipelineState);

            ShaderParametersRef descSets[] = { viewParameters, instanceParameters };
            cmdList->cmdBindDescriptorsSets(cmdBuffer, sceneDebugLinesPipelineContext, descSets);
            std::pair<String, std::any> pushConsts[] = {
                {TCHAR("ptSize"), 1.0f}
            };
            cmdList->cmdPushConstants(cmdBuffer, sceneDebugLinesPipelineContext, pushConsts);
            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, sceneEntity.meshAsset->getTbnVertexBuffer(), 0);
            // Drawing with instance from one of batch as we do not care about material idx
            cmdList->cmdDrawVertices(cmdBuffer, 0, uint32(sceneEntity.meshAsset->tbnVerts.size()), sceneEntity.instanceParamIdx[0]);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
#endif
    rendererModule->getRenderManager()->preparePipelineContext(&drawGridDTestPipelineCntxt, backFramebufferRts);
    if (bDrawGrid)
    {
        // Resetting viewport as we use mvp again
        viewport.minBound.x = 0;
        viewport.minBound.y = ApplicationSettings::screenSize.get().y;
        viewport.maxBound.x = ApplicationSettings::screenSize.get().x;
        viewport.maxBound.y = 0;

        SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawGrid);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBeginRenderPass(cmdBuffer, drawGridDTestPipelineCntxt, scissor, debugSceneDrawAdditionalProps, clearValues);
        {
            std::vector<std::pair<String, std::any>> pushCnsts{
                {        TCHAR("gridCellSize"),         gridCellSize},
                {      TCHAR("gridExtendSize"),       gridExtendSize},
                                                           {TCHAR("cellMinPixelCoverage"), cellMinPixelCoverage},
                                                             {           TCHAR("thinColor"),  Vector4D(thinColor)},
                                                               {          TCHAR("thickColor"), Vector4D(thickColor)}
            };
            GraphicsPipelineState pipelineState;
            pipelineState.pipelineQuery = { EPolygonDrawMode::Fill, ECullingMode::None };
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawGridDTestPipelineCntxt, pipelineState);
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawGridDTestPipelineCntxt, { viewParameters });
            cmdList->cmdPushConstants(cmdBuffer, drawGridDTestPipelineCntxt, pushCnsts);

            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadRectVertexIndexBuffers().first, 0);
            cmdList->cmdBindIndexBuffer(cmdBuffer, GlobalBuffers::getQuadRectVertexIndexBuffers().second);

            cmdList->cmdDrawIndexed(cmdBuffer, 0, 6);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }

    const IRenderTargetTexture *lightingRtPtr = frameResources[swapchainIdx].lightingPassRt;
    rendererModule->getRenderManager()->preparePipelineContext(&overBlendedQuadPipelineContext, { &lightingRtPtr, 1 });
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawCameraGizmoRT);

        RenderPassAdditionalProps drawOverlay;
        drawOverlay.colorAttachmentLoadOp = EAttachmentOp::LoadOp::Load;

        const Int2D margin(10, 10);

        Vector2D viewportSize
            = (Vector2D(camGizmoColorTexture->getTextureSize()) / Vector2D(3840, 2160)) * Vector2D(ApplicationSettings::screenSize.get());
        viewport.minBound = Int2D(0 + margin.x, ApplicationSettings::screenSize.get().y - int32(viewportSize.y()) - margin.y);
        viewport.maxBound = viewport.minBound + Int2D(viewportSize.x(), viewportSize.y());

        scissor = viewport;

        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBeginRenderPass(cmdBuffer, overBlendedQuadPipelineContext, viewport, drawOverlay, clearValues);
        {
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, overBlendedQuadPipelineContext, { backfaceFillQueryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, overBlendedQuadPipelineContext, camRTParams);

            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);

            cmdList->cmdDrawVertices(cmdBuffer, 0, 3);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);
    }
}

void ExperimentalEnginePBR::tickEngine()
{
    TestGameEngine::tickEngine();
    updateCameraParams();
    setupLightShaderData();

    if (application->inputSystem->isKeyPressed(Keys::ONE))
    {
        frameVisualizeId = 0;
    }
    else if (application->inputSystem->isKeyPressed(Keys::TWO))
    {
        frameVisualizeId = 1;
    }
    else if (application->inputSystem->isKeyPressed(Keys::THREE))
    {
        frameVisualizeId = 2;
    }
    else if (application->inputSystem->isKeyPressed(Keys::FOUR))
    {
        frameVisualizeId = 3;
    }

    if (application->inputSystem->keyState(Keys::LMB)->keyWentDown && !getImGuiManager().capturedInputs()
        && !application->windowManager->getMainWindow()->isMinimized())
    {
        QuantShortBox2D windowArea = application->windowManager->getMainWindow()->windowClientRect();
        Vector2D windowOrigin{ float(windowArea.minBound.x), float(windowArea.minBound.y) };
        Vector2D mouseCoord = Vector2D(
                                  application->inputSystem->analogState(AnalogStates::AbsMouseX)->currentValue,
                                  application->inputSystem->analogState(AnalogStates::AbsMouseY)->currentValue
                              )
                              - windowOrigin;
        mouseCoord /= Vector2D(ApplicationSettings::surfaceSize.get());
        LOG_DEBUG("ExperimentalEnginePBR", "mouse coord (%f, %f)", mouseCoord.x(), mouseCoord.y());
        if (mouseCoord.x() >= 0 && mouseCoord.x() <= 1.0f && mouseCoord.y() >= 0 && mouseCoord.y() <= 1.0f)
        {
            Vector3D worldFwd = camera.screenToWorldFwd(mouseCoord);
            std::vector<GridEntity> entities;
            if (sceneVolume.raycast(entities, camera.translation(), worldFwd, 2000))
            {
                selection = entities.front();
            }
            else
            {
                selection.type = GridEntity::Invalid;
            }
        }
    }

    if (!application->windowManager->getMainWindow()->isMinimized())
    {
        ENQUEUE_COMMAND(TickFrame)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
#if SHADOWS_USE_CULLED_DRAW_CMDS
                setupLightSceneDrawCmdsBuffer(cmdList, graphicsInstance);
#endif
                frameRender(cmdList, graphicsInstance, graphicsHelper);
            }
        );
    }

    // Do the resize after frame rendering so next frame will be updated
    if (renderSize != ApplicationSettings::screenSize.get())
    {
        ENQUEUE_COMMAND(WritingDescs)
        (
            [this](class IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
            {
                GBuffers::onScreenResized(renderSize);
                resizeLightingRts(renderSize);
                reupdateTextureParamsOnResize();
                ApplicationSettings::screenSize.set(renderSize);
            }
        );
    }

    tempTestPerFrame();
}

int32 ExperimentalEnginePBR::layerDepth() const { return 0; }

int32 ExperimentalEnginePBR::sublayerDepth() const { return 0; }

class TestWidget : public WidgetBase
{
private:
    Short2D origin = { 300, 300 };
    Short2D halfExtent = { 75, 50 };
    Color colors[4] = { ColorConst::GREEN, ColorConst::GREEN, ColorConst::GREEN, ColorConst::GREEN };
    Color color = ColorConst::GREEN;

    SharedPtr<TestWidget> content;

public:
    struct WgArguments
    {
        Short2D origin = { 300, 300 };
        Short2D halfExtent = { 75, 50 };
        Color color = ColorConst::GREEN;
        // 1 or 2 or 3 or 0
        uint32 style;
    };

    void construct(WgArguments args)
    {
        origin = args.origin;
        halfExtent = args.halfExtent;
        color = args.color;
        colors[0] = args.color;
        colors[1] = args.color;
        colors[2] = args.color;
        colors[3] = args.color;

        if (args.style == 0)
        {
            return;
        }

        if (args.style == 2)
        {
            TestWidget::WgArguments childArgs = args;
            childArgs.origin = halfExtent * Short2D(2);
            childArgs.style = 0;
            childArgs.color = Color{ 255, 0, 0, 130 };
            auto wg = std::make_shared<TestWidget>();
            wg->construct(childArgs);
            content = wg;
        }
        if (args.style == 3)
        {
            TestWidget::WgArguments childArgs = args;
            childArgs.origin = Short2D(halfExtent.x + 2 * origin.x, halfExtent.y);
            childArgs.color = ColorConst::BLUE;
            childArgs.style = 0;
            auto wg = std::make_shared<TestWidget>();
            wg->construct(childArgs);
            content = wg;

            childArgs.origin = Short2D(halfExtent.x, 2 * halfExtent.y + origin.y);
            childArgs.color = ColorConst::RED;
            wg = std::make_shared<TestWidget>();
            wg->construct(childArgs);
            content->content = wg;

            childArgs.origin = Short2D(halfExtent.x - 2 * origin.x, halfExtent.y);
            childArgs.color = ColorConst::GRAY;
            wg = std::make_shared<TestWidget>();
            wg->construct(childArgs);
            content->content->content = wg;

            childArgs.origin = Short2D(halfExtent.x + origin.x, 0);
            childArgs.color = ColorConst::CYAN;
            wg = std::make_shared<TestWidget>();
            wg->construct(childArgs);
            content->content->content->content = wg;
        }
    }

protected:
    void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) override
    {
        WidgetGeom &geom = geomTree[thisId];
        geom.box = { origin - halfExtent, origin + halfExtent };
        if (content)
        {
            WidgetGeomId childId = geomTree.add(WidgetGeom{ .widget = content }, thisId);
            content->rebuildWidgetGeometry(childId, geomTree);
        }
    }

public:
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override
    {
        const QuantShortBox2D &box = geomTree[thisId].box;
        context.drawBox(box, nullptr, clipBound, colors);
        if (content)
        {
            WidgetGeomId childId = geomTree.getChildren(thisId, false)[0];
            context.beginLayer();
            // Below is right way but we are not using traditional geometry
            // content->drawWidget(clipBound.getIntersectionBox(geomTree[childId].box), childId, geomTree, context);
            content->drawWidget(geomTree[childId].box, childId, geomTree, context);
            context.endLayer();
        }
    }
    bool hasWidget(SharedPtr<WidgetBase> widget) const override { return content && (content == widget || content->hasWidget(widget)); }

    void tick(float /*timeDelta*/) override {}

    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override
    {
        return content ? content->inputKey(key, state, inputSystem) : EInputHandleState::NotHandled;
    }
    EInputHandleState analogKey(AnalogStates::StateKeyType /*key*/, AnalogStates::StateInfoType /*state*/, const InputSystem * /*inputSystem*/)
    {
        return EInputHandleState::NotHandled;
    }

    void mouseEnter(Short2D /*absPos*/, Short2D /*widgetRelPos*/, const InputSystem * /*inputSystem*/) override
    {
        colors[0] = ColorConst::random();
        colors[1] = ColorConst::random();
        colors[2] = ColorConst::random();
        colors[3] = ColorConst::random();
    }

    void mouseMoved(Short2D /*absPos*/, Short2D /*widgetRelPos*/, const InputSystem * /*inputSystem*/) override {}

    void mouseLeave(Short2D /*absPos*/, Short2D /*widgetRelPos*/, const InputSystem * /*inputSystem*/) override
    {
        colors[0] = color;
        colors[1] = color;
        colors[2] = color;
        colors[3] = color;
    }
};

void ExperimentalEnginePBR::draw(class ImGuiDrawInterface *drawInterface)
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

        if (!ImGui::Begin("Settings", &bSettingOpen))
        {
            ImGui::End();
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

            const InputAnalogState *rmxState = application->inputSystem->analogState(AnalogStates::RelMouseX);
            const InputAnalogState *rmyState = application->inputSystem->analogState(AnalogStates::RelMouseY);
            const InputAnalogState *amxState = application->inputSystem->analogState(AnalogStates::AbsMouseX);
            const InputAnalogState *amyState = application->inputSystem->analogState(AnalogStates::AbsMouseY);
            ImGui::Text(
                "Cursor pos (%.0f, %.0f) Delta (%0.1f, %0.1f)", amxState->currentValue, amyState->currentValue, rmxState->currentValue,
                rmyState->currentValue
            );
            SharedPtr<WgWindow> wnd = application->getHoveringWindow();
            if (wnd)
            {
                Short2D wndRelPos = wnd->screenToWgWindowSpace(Short2D(amxState->currentValue, amyState->currentValue));
                ImGui::Text(
                    "Cursor pos in window %s (%d, %d)", TCHAR_TO_UTF8(wnd->getAppWindow()->getWindowName().getChar()), wndRelPos.x, wndRelPos.y
                );
            }
            if (ImGui::Button("New Window"))
            {
                copat::fireAndForget(
                    [this]() -> copat::JobSystemEnqTask<copat::EJobThreadType::MainThread>
                    {
                        static int32 count = 1;
                        TestWidget::WgArguments args;
                        args.halfExtent = { 200, 110 };
                        args.origin = { 213, 120 };
                        args.style = (count % 3) + 1;
                        auto wg = std::make_shared<TestWidget>();
                        wg->construct(args);
                        application->createWindow(renderSize, ("TestWindow" + String::toString(count)).c_str(), nullptr)->setContent(wg);
                        count++;
                        co_return;
                    }
                );
            }

            if (ImGui::CollapsingHeader("Camera"))
            {
                {
                    const char *proj[] = { "Perspective", "Orthographic" };
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

                ImGui::DragFloat3("Translation", &cameraTranslation[0], 1.f);

                if (ImGui::DragFloat3("Rotation", reinterpret_cast<float *>(&cameraRotation), 1.f, 0.0f, 360.0f))
                {
                    updateCamGizmoViewParams();
                    ENQUEUE_COMMAND(CameraGizmoUpdate)
                    (
                        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *)
                        {
                            updateCamGizmoCapture(cmdList, graphicsInstance);
                        }
                    );
                }
            }

            if (ImGui::CollapsingHeader("Rendering"))
            {
                {
                    const char *resolutions[] = { "1280x720", "1920x1080", "2560x1440", "3840x2160" };
                    static int currRes = 0;
                    ImGui::Combo("RenderSize", &currRes, resolutions, ARRAY_LENGTH(resolutions));
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

                {
                    const char *bufferMode[] = { "Lit", "Unlit", "Normal", "AO", "Roughness", "metallic", "Depth" };
                    ImGui::Combo("Visualize buffer", &frameVisualizeId, bufferMode, ARRAY_LENGTH(bufferMode));
                }

                ImGui::Separator();
                if (ImGui::InputFloat("Exposure", &exposure, 1.0f, 10.f, "%.1f"))
                {
                    (*dirLight.paramCollection)->setFloatParam(TCHAR("exposure"), exposure);
                }

                if (ImGui::InputFloat("Gamma", &gamma, 1.0f, 4.f, "%.1f"))
                {
                    (*dirLight.paramCollection)->setFloatParam(TCHAR("gamma"), gamma);
                }

                if (ImGui::Combo("Env Map", &selectedEnv, envMapNames.data(), int32(envMapNames.size())))
                {
                    reupdateEnvMap();
                }

                if (ImGui::CollapsingHeader("Cascades"))
                {
                    bool bAnyChanged = false;
                    int32 idx = 0;
                    for (CascadeData &cascade : dirLight.cascades)
                    {
                        String label = TCHAR("Cascade TCHAR(") + String::toString(idx);
                        bAnyChanged
                            = bAnyChanged || ImGui::DragFloat(TCHAR_TO_UTF8(label.getChar()), &cascade.frustumFract, 0.005f, 0.0f, 1.0f);
                        ++idx;
                    }

                    if (bAnyChanged)
                    {
                        dirLight.normalizeCascadeCoverage();
                    }
                }
                if (ImGui::TreeNode("Shadow flags"))
                {
                    ImGui::CheckboxFlags("Draw back face", &shadowFlags, PBRShadowFlags::DrawingBackface);
                    ImGui::TreePop();
                    ImGui::Separator();
                }
#if DEV_BUILD
                ImGui::Checkbox("Draw TBN", &bDrawTbn);
#endif
                {
                    const char *renderFlagTexts[] = { "None",
                                                      "Disable Ambient Light",
                                                      "Disable Directional Light",
                                                      "Disable Ambient and Directional",
                                                      "Disable Shadows",
                                                      "Draw cascades" };
                    ImGui::Combo("Render option", &renderFlags, renderFlagTexts, ARRAY_LENGTH(renderFlagTexts));
                }

                ImGui::Separator();
                ImGui::Checkbox("Show Grid", &bDrawGrid);
                ImGui::InputFloat("Extent", &gridExtendSize, 10, 100);
                ImGui::InputFloat("Cell Size", &gridCellSize, 5, 20);
                ImGui::ColorEdit4("Minor grid color", reinterpret_cast<float *>(&thinColor));
                ImGui::ColorEdit4("Major grid color", reinterpret_cast<float *>(&thickColor));
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
                    if (ImGui::DragFloat3("Direction", reinterpret_cast<float *>(&dirLight.direction), 0.5f, -180.f, 180.f, "%.4f"))
                    {
                        dirLight.update();
                    }

                    if (ImGui::ColorEdit3("Color", reinterpret_cast<float *>(&dirLight.lightcolor)))
                    {
                        Vector4D param{ dirLight.lightcolor };
                        param.w() = dirLight.lumen;
                        (*dirLight.paramCollection)->setVector4Param(TCHAR("lightColor_lumen"), param);
                    }

                    if (ImGui::InputFloat("Lumen", &dirLight.lumen, 1.0f, 10.f, "%.1f"))
                    {
                        Vector4D param{ dirLight.lightcolor };
                        param.w() = dirLight.lumen;
                        (*dirLight.paramCollection)->setVector4Param(TCHAR("lightColor_lumen"), param);
                    }
                }
                ImGui::TreePop();
            }

            if (ImGui::CollapsingHeader("Texture Histogram"))
            {
                if (selectedTexture != 0)
                {
                    ImGui::Image(textures[selectedTexture - 1]->getTexture()->getTextureResource().get(), ImVec2(64, 64));
                    ImGui::SameLine();
                }
                if (ImGui::Combo("Textures", &selectedTexture, textureNames.data(), int32(textureNames.size())))
                {
                    if (selectedTexture != 0)
                    {
                        ImageUtils::calcHistogramRGB(
                            histogram[0].data(), histogram[1].data(), histogram[2].data(), 32,
                            reinterpret_cast<const uint8 *>(textures[selectedTexture - 1]->getPixelData().data()),
                            textures[selectedTexture - 1]->getTexture()->getTextureSize().x,
                            textures[selectedTexture - 1]->getTexture()->getTextureSize().y, 4
                        );
                    }
                }

                if (selectedTexture != 0)
                {
                    ImPlot::SetNextPlotLimits(0, 255, 0, 1.0, ImGuiCond_::ImGuiCond_Once);
                    if (ImPlot::BeginPlot(
                            "Texture Histogram", 0, 0, ImVec2(-1, 0), 0, ImPlotAxisFlags_::ImPlotAxisFlags_Lock,
                            ImPlotAxisFlags_::ImPlotAxisFlags_Lock
                        ))
                    {
                        ImPlot::SetNextFillStyle(LinearColorConst::RED, 1.0f);
                        ImPlot::PlotShaded("Red", histogram[0].data(), int32(histogram[0].size()), 0.0f,
                                           8); // 256/ binCount(32)

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
    ImGui::SetNextWindowSize(ImVec2(165, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 165, 0), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, LinearColor(0, 0, 0, 0.6f));
    ImGui::Begin(
        "FPS", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar
    );
    ImGui::Text("%.3f ms(%.1f FPS)", 1000.0f * application->timeData.smoothedDeltaTime, 1 / application->timeData.smoothedDeltaTime);
    ImGui::End();
    ImGui::PopStyleColor();
}

void ExperimentalEnginePBR::drawSelectionWidget(class ImGuiDrawInterface */*drawInterface*/)
{
    if (ImGui::CollapsingHeader("Selection"))
    {
        switch (selection.type)
        {
        case GridEntity::Entity:
        {
            PBRSceneEntity &entity = sceneData[selection.idx];
            String name = entity.name;
            ImGui::Text("Selected Entity : %s", TCHAR_TO_UTF8(name.getChar()));

            if (ImGui::CollapsingHeader("Transform"))
            {
                AABB currentBound = getBounds(selection);
                bool bTransformChanged = ImGui::DragFloat3("Translation", reinterpret_cast<float *>(&entity.transform.getTranslation()), 1.f);
                bTransformChanged = ImGui::DragFloat3("Rotation", reinterpret_cast<float *>(&entity.transform.getRotation()), 1.f, 0.0f, 360.0f)
                                    || bTransformChanged;

                if (ImGui::DragFloat3("Scale", reinterpret_cast<float *>(&entity.transform.getScale()), 0.05f))
                {
                    bTransformChanged = true;
                    for (uint32 i = 0; i < 3; ++i)
                    {
                        if (Math::isEqual(entity.transform.getScale()[i], 0.0f))
                        {
                            entity.transform.getScale()[i] += 0.1f;
                        }
                    }
                }

                if (bTransformChanged)
                {
                    entity.updateInstanceParams(instanceParameters);

                    AABB newBound = getBounds(selection);
                    sceneVolume.updateBounds(selection, currentBound, newBound);
                    if (sceneVolume.findIntersection(newBound, true).empty())
                    {
                        sceneVolume.addedNewObject(selection);
                    }
                }
            }

            if (ImGui::TreeNode("Materials"))
            {
                for (uint32 i = 0; i < entity.meshBatchProps.size(); ++i)
                {
                    String materialName = entity.meshAsset->meshBatches[i].name.empty() ? TCHAR("Material ") + String::toString(i)
                                                                                        : entity.meshAsset->meshBatches[i].name;
                    if (ImGui::TreeNode(TCHAR_TO_UTF8(materialName.getChar())))
                    {
                        bool bAnyChanged = false;
                        PBRSceneEntity::BatchProperties &props = entity.meshBatchProps[i];
                        bAnyChanged = ImGui::ColorEdit3("Color", reinterpret_cast<float *>(&props.color)) || bAnyChanged;
                        bAnyChanged = ImGui::DragFloat("Roughness", &props.roughness, 0.05f, 0.0f, 1.0f) || bAnyChanged;
                        bAnyChanged = ImGui::DragFloat("Metallic", &props.metallic, 0.05f, 0.0f, 1.0f) || bAnyChanged;
                        bAnyChanged = ImGui::DragFloat2("UV scaling", reinterpret_cast<float *>(&props.uvScale), 0.5f, 0.01f) || bAnyChanged;
                        if (bAnyChanged)
                        {
                            entity.updateMaterialParams(sceneShaderUniqParams[props.pipeline], tex2dToBindlessIdx, i);
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            break;
        }
        case GridEntity::SpotLight:
        {
            SpotLight &entity = sceneSpotLights[selection.idx];
            String name = entity.name;
            ImGui::Text("Selected Spot light : %s", TCHAR_TO_UTF8(name.getChar()));

            bool bNeedsUpdate = false;
            if (ImGui::CollapsingHeader("Transform"))
            {
                AABB currentBound = getBounds(selection);
                bool bTransformChanged = ImGui::DragFloat3("Translation", reinterpret_cast<float *>(&entity.transform.getTranslation()), 1.0f);
                bTransformChanged
                    = ImGui::DragFloat3("Direction", reinterpret_cast<float *>(&entity.transform.getRotation()), 1.0f, 0.0f, 360.0f)
                      || bTransformChanged;

                if (bTransformChanged)
                {
                    AABB newBound = getBounds(selection);
                    sceneVolume.updateBounds(selection, currentBound, newBound);
                    if (sceneVolume.findIntersection(newBound, true).empty())
                    {
                        sceneVolume.addedNewObject(selection);
                    }
                }
                bNeedsUpdate = bTransformChanged;
            }

            if (ImGui::ColorEdit3("Color", reinterpret_cast<float *>(&entity.lightcolor)))
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
            PointLight &entity = scenePointLights[selection.idx];
            String name = entity.name;
            ImGui::Text("Selected Point light : %s", TCHAR_TO_UTF8(name.getChar()));

            bool bNeedsUpdate = false;

            AABB currentBound = getBounds(selection);
            if (ImGui::DragFloat3("Translation", reinterpret_cast<float *>(&entity.lightPos), 1.0f))
            {
                AABB newBound = getBounds(selection);
                sceneVolume.updateBounds(selection, currentBound, newBound);
                if (sceneVolume.findIntersection(newBound, true).empty())
                {
                    sceneVolume.addedNewObject(selection);
                }
                bNeedsUpdate = true;
            }

            if (ImGui::ColorEdit3("Color", reinterpret_cast<float *>(&entity.lightcolor)))
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

void ExperimentalEnginePBR::tempTest() {}

void ExperimentalEnginePBR::tempTestPerFrame()
{
    // CoreObjectGC &gc = ICoreObjectsModule::get()->getGC();
    // gc.collect(0.016f);
}

void ExperimentalEnginePBR::tempTestQuit() {}

// TestGameEngine *GameEngineWrapper::createEngineInstance()
//{
//     static SharedPtr<ExperimentalEnginePBR> engineInst = std::make_shared<ExperimentalEnginePBR>();
//     return engineInst.get();
// }
#endif