/*!
 * \file ExperimentalEngineGoochModel.cpp
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

#include "../Assets/Asset/StaticMeshAsset.h"
#include "../Assets/Asset/TextureAsset.h"
#include "../Core/Types/Textures/RenderTargetTextures.h"
#include "../Core/Types/Textures/Texture2D.h"
#include "../Core/Types/Textures/TexturesBase.h"
#include "ApplicationInstance.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"
#include "Widgets/ImGui/IImGuiLayer.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/ImGui/ImGuiLib/implot.h"
#include "Core/GBuffers.h"
#include "IApplicationModule.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keys.h"
#include "Math/Math.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector3D.h"
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
#include "RenderApi/Shaders/EngineShaders/GoochModelShader.h"
#include "RenderApi/Shaders/EngineShaders/SingleColorShader.h"
#include "Types/Camera/Camera.h"
#include "Types/Colors.h"
#include "Types/Containers/ReferenceCountPtr.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Transform3D.h"
#include "WindowManager.h"
#include "ApplicationSettings.h"
#include "FontManager.h"
#include "GenericAppWindow.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/LFS/PathFunctions.h"

#include "Math/MathGeom.h"
#include "RenderInterface/Resources/Pipelines.h"
#include <array>
#include <map>
#include <random>

struct AOS
{
    Vector4D a;
    Vector2D b;
    Vector2D c[4];
};

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
        for (std::pair<std::vector<LineSegment>, Color> &color : perGroup)
        {
            color.second = ColorConst::random();
        }

        int32 threadNum = count / 2;
        for (int32 t = 0; t < threadNum; t++)
        {
            std::pair<std::vector<LineSegment>, Color> &threadIndices = perThreadIndices.emplace_back();
            threadIndices.second = ColorConst::random();

            float step = 0;
            for (int32 h = 2; h <= count; h *= 2)
            {
                step++;

                int32 flipStartIdx = (t / (h / 2)) * h;
                int32 flipOffset = (t % (h / 2));

                int32 flipLhsIdx = flipStartIdx + flipOffset;
                int32 flipRhsIdx = flipStartIdx + h - flipOffset - 1;

                LineSegment *segment = &threadIndices.first.emplace_back();
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

struct SceneEntity
{
    Transform3D transform;
    class StaticMeshAsset *meshAsset;

    std::vector<LinearColor> meshBatchColors;

    // Generated
    // Per mesh batch instance and shader param index
    // since material index is within the instance data
    std::vector<uint32> instanceParamIdx;
    std::vector<uint32> batchShaderParamIdx;

    void updateInstanceParams(ShaderParametersRef &shaderParams, uint32 batchIdx);
    void updateInstanceParams(ShaderParametersRef &shaderParams)
    {
        for (uint32 i = 0; i < meshBatchColors.size(); ++i)
        {
            updateInstanceParams(shaderParams, i);
        }
    }
    void updateMaterialParams(
        ShaderParametersRef &shaderParams, const std::unordered_map<ImageResourceRef, uint32> &tex2dToBindlessIdx, uint32 batchIdx
    ) const;
};

void SceneEntity::updateInstanceParams(ShaderParametersRef &shaderParams, uint32 batchIdx)
{
    InstanceData gpuInstance;
    gpuInstance.model = transform.getTransformMatrix();
    gpuInstance.invModel = transform.getTransformMatrix().inverse();
    gpuInstance.shaderUniqIdx = batchShaderParamIdx[batchIdx];

    shaderParams->setBuffer(TCHAR("instances"), gpuInstance, instanceParamIdx[batchIdx]);
}

void SceneEntity::updateMaterialParams(
    ShaderParametersRef &shaderParams, const std::unordered_map<ImageResourceRef, uint32> & /*tex2dToBindlessIdx*/, uint32 batchIdx
) const
{
    SingleColorMeshData singleColorMeshData;
    singleColorMeshData.meshColor = meshBatchColors[batchIdx];
    singleColorMeshData.metallic = 0;
    singleColorMeshData.roughness = 0;
    shaderParams->setBuffer(TCHAR("meshData"), singleColorMeshData, batchShaderParamIdx[batchIdx]);
}

struct FrameResource
{
    std::vector<SemaphoreRef> usageWaitSemaphore;
    RenderTargetTexture *lightingPassRt;
    RenderTargetTexture *lightingPassResolved;
    FenceRef recordingFence;
};

class ExperimentalEngineGoochModel
    : public TestGameEngine
    , public IImGuiLayer
{
    SamplerRef nearestFiltering = nullptr;
    SamplerRef linearFiltering = nullptr;
    // TODO(Jeslas) : Cubic filtering not working check new drivers or log bug in nvidia
    // SharedPtr<class SamplerInterface> cubicFiltering = nullptr;
    void createImages(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyImages();
    // Global parameters
    // Asset's data
    std::unordered_map<ImageResourceRef, uint32> tex2dToBindlessIdx;
    // offset in count, in scene
    std::unordered_map<const MeshAsset *, std::pair<uint32, uint32>> meshVertIdxOffset;

    // Scene data
    std::vector<SceneEntity> sceneData;
    BufferResourceRef sceneVertexBuffer;
    BufferResourceRef sceneIndexBuffer;
    BufferResourceRef allEntityDrawCmds;
    // Offset in bytes, Count in size
    std::unordered_map<const LocalPipelineContext *, std::pair<uint32, uint32>> pipelineToDrawCmdOffsetCount;
    void createDrawCmdsBuffer(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyDrawCmdsBuffer();

    std::vector<struct GoochModelLightData> sceneLightData;
    std::vector<ShaderParametersRef> lightData;
    ShaderParametersRef lightCommon;
    SwapchainBufferedResource<ShaderParametersRef> lightTextures;
    ShaderParametersRef viewParameters;
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
    SwapchainBufferedResource<ShaderParametersRef> drawQuadDepthDescs;
    SwapchainBufferedResource<ShaderParametersRef> drawLitColorsDescs;

    void
    createShaderParameters(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void setupShaderParameterParams(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyShaderParameters();

    void resizeLightingRts(const Size2D &size);
    void reupdateTextureParamsOnResize();

    // Shader pipeline resources
    RenderPassClearValue clearValues;

    void createFrameResources(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyFrameResources();

    LocalPipelineContext drawSmPipelineContext;

    LocalPipelineContext drawGoochPipelineContext;

    LocalPipelineContext drawQuadPipelineContext;
    LocalPipelineContext resolveLightRtPipelineContext;

    ShaderParametersRef clearInfoParams;
    LocalPipelineContext clearQuadPipelineContext;

    class TextureBase *writeTexture = nullptr;
    ShaderParametersRef testComputeParams;
    LocalPipelineContext testComputePipelineContext;

    void getPipelineForSubpass();
    void clearPipelineContexts();

    std::vector<FrameResource> frameResources;
    void
    createPipelineResources(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyPipelineResources();

    // End shader pipeline resources

    // Test compute
    bool bAnimateX;
    bool bAnimateY;
    bool bCompute = false;
    float timeAccum = 0;
    uint32 texturesCount;
    uint32 testBindlessTextureIdx;

    std::vector<ShortSizeBox2D> boxes;

    String textToRender;
    LinearColor textCol = LinearColorConst::BLUE;
    int32 wrapSize = 60;
    int32 textHeight = 32;
    QuantizedBox2D textBB;
    LocalPipelineContext imguiShaderCntxt;
    ShaderParametersRef textRenderParams;
    uint32 textVertCount, textIdxCount;
    BufferResourceRef textVertsBuffer;
    BufferResourceRef textIndexBuffer;
    void
    createTextRenderResources(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyTextRenderResources();
    void updateTextRenderData(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);

    int32 frameVisualizeId = 0; // 0 color 1 normal 2 depth
    Size2D renderSize{ 1280, 720 };
    ECameraProjection projection = ECameraProjection::Perspective;

protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

    void startUpRenderInit(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void renderQuit();
    void frameRender(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);

    void tempTest();
    void tempTestPerFrame();
    /* IImGuiLayer Implementation */
public:
    int32 layerDepth() const override;
    int32 sublayerDepth() const override;
    void draw(class ImGuiDrawInterface *drawInterface) override;
    /* end overrides */
};

void ExperimentalEngineGoochModel::tempTest() {}

void ExperimentalEngineGoochModel::tempTestPerFrame() {}

void ExperimentalEngineGoochModel::createImages(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    SamplerCreateInfo samplerCI{
        .filtering = ESamplerFiltering::Nearest,
        .mipFiltering = ESamplerFiltering::Nearest,
        .tilingMode = { ESamplerTilingMode::Repeat, ESamplerTilingMode::Repeat, ESamplerTilingMode::Repeat },
        .mipLodRange = { 0, float(GlobalRenderVariables::MIN_SAMPLINE_MIP_LEVEL.get()) },
        .resourceName = TCHAR("NearestSampler")
    };
    nearestFiltering = graphicsHelper->createSampler(graphicsInstance, samplerCI);
    nearestFiltering->init();

    samplerCI.resourceName = TCHAR("LinearSampler");
    samplerCI.filtering = samplerCI.mipFiltering = ESamplerFiltering::Linear;
    linearFiltering = graphicsHelper->createSampler(graphicsInstance, samplerCI);
    linearFiltering->init();

    Texture2DRWCreateParams createParam;
    createParam.textureSize = Size2D(512);
    createParam.mipCount = 1;
    createParam.textureName = TCHAR("Compute Write");
    createParam.format = EPixelDataFormat::RGBA_U8_Norm;
    createParam.bIsWriteOnly = false;
    writeTexture = TextureBase::createTexture<Texture2DRW>(createParam);
}

void ExperimentalEngineGoochModel::destroyImages()
{
    TextureBase::destroyTexture<Texture2DRW>(writeTexture);
    writeTexture = nullptr;
    nearestFiltering.reset();
    linearFiltering.reset();
}

void ExperimentalEngineGoochModel::createDrawCmdsBuffer(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    // Setup all draw commands, Instance idx for each batch and its material idx
    std::vector<DrawIndexedIndirectCommand> drawCmds;
    {
        // Using set to sort by batch to use instanced draw
        std::unordered_map<LocalPipelineContext *, std::map<const MeshAsset *, std::set<std::pair<uint32, uint32>>>>
            pipelineToMeshToBatchEntityIdx;
        uint32 entityIdx = 0;
        for (SceneEntity &entity : sceneData)
        {
            entity.instanceParamIdx.resize(entity.meshBatchColors.size());
            entity.batchShaderParamIdx.resize(entity.meshBatchColors.size());

            for (uint32 meshBatchIdx = 0; meshBatchIdx < entity.meshBatchColors.size(); ++meshBatchIdx)
            {
                pipelineToMeshToBatchEntityIdx[&drawSmPipelineContext][entity.meshAsset].insert(std::pair<uint32, uint32>{ meshBatchIdx,
                                                                                                                           entityIdx });
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
        for (SceneEntity &entity : sceneData)
        {
            for (uint32 meshBatchIdx = 0; meshBatchIdx < entity.meshBatchColors.size(); ++meshBatchIdx)
            {
                entity.updateInstanceParams(instanceParameters, meshBatchIdx);
                entity.updateMaterialParams(sceneShaderUniqParams[&drawSmPipelineContext], tex2dToBindlessIdx, meshBatchIdx);
            }
            entityIdx++;
        }
    }

    ENQUEUE_COMMAND(CreateAllEntityDrawCmds)
    (
        [drawCmds, this](class IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            cmdList->copyToBuffer(allEntityDrawCmds, 0, drawCmds.data(), uint32(allEntityDrawCmds->getResourceSize()));
        }
    );
}

void ExperimentalEngineGoochModel::destroyDrawCmdsBuffer() { allEntityDrawCmds.reset(); }

void ExperimentalEngineGoochModel::createScene()
{
    StaticMeshAsset *cube = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Cube.obj")));
    StaticMeshAsset *sphere = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Sphere.obj")));
    StaticMeshAsset *cylinder = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Cylinder.obj")));
    StaticMeshAsset *cone = static_cast<StaticMeshAsset *>(assetManager.getOrLoadAsset(TCHAR("Cone.obj")));
    std::array<StaticMeshAsset *, 4> assets{ cube, sphere, cylinder, cone };
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

            for (uint32 idx = 0; idx < 5; ++idx)
            {
                SceneEntity entity;
                entity.meshAsset = assets[std::rand() % assets.size()];
                entity.transform.setTranslation(
                    offset + Vector3D(distribution(generator) * 400, distribution(generator) * 400, distribution1(generator) * 100 + 50)
                );
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

void ExperimentalEngineGoochModel::createSceneRenderData(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    uint32 totalVertexLen = 0;
    uint32 totalIdxLen = 0;

    for (const SceneEntity &entity : sceneData)
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

void ExperimentalEngineGoochModel::destroyScene()
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

void ExperimentalEngineGoochModel::createShaderParameters(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    const PipelineBase *smPipeline = static_cast<const GraphicsPipelineBase *>(drawSmPipelineContext.getPipeline());
    // Since view data and other view related data are at set 0
    viewParameters
        = graphicsHelper->createShaderParameters(graphicsInstance, smPipeline->getParamLayoutAtSet(ShaderParameterUtility::VIEW_UNIQ_SET));
    viewParameters->setResourceName(TCHAR("View"));
    // All vertex type's instance data(we have only static)
    instanceParameters
        = graphicsHelper->createShaderParameters(graphicsInstance, smPipeline->getParamLayoutAtSet(ShaderParameterUtility::INSTANCE_UNIQ_SET));
    instanceParameters->setResourceName(TCHAR("StaticVertexInstances"));
    ShaderParametersRef singleColShaderParams
        = graphicsHelper->createShaderParameters(graphicsInstance, smPipeline->getParamLayoutAtSet(ShaderParameterUtility::SHADER_UNIQ_SET));
    singleColShaderParams->setResourceName(TCHAR("SingleColorShaderParams"));
    sceneShaderUniqParams[&drawSmPipelineContext] = singleColShaderParams;

    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    uint32 swapchainCount = windowCanvas->imagesCount();
    lightTextures.setNewSwapchain(windowCanvas);
    drawQuadTextureDescs.setNewSwapchain(windowCanvas);
    drawQuadNormalDescs.setNewSwapchain(windowCanvas);
    drawQuadDepthDescs.setNewSwapchain(windowCanvas);
    drawLitColorsDescs.setNewSwapchain(windowCanvas);

    // Light related descriptors
    // as 2 and 3 are textures and light data
    const GraphicsResource *goochModelDescLayout = drawGoochPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    lightCommon = graphicsHelper->createShaderParameters(graphicsInstance, goochModelDescLayout, { 2, 3 });
    lightCommon->setResourceName(TCHAR("LightCommon"));
    const uint32 lightDataCount = uint32(Math::ceil(sceneLightData.size() / float(ARRAY_LENGTH(GoochModelLightArray::lights))));
    lightData.resize(lightDataCount);
    for (uint32 i = 0; i < lightDataCount; ++i)
    {
        // as 1 and 2 are light common and textures
        lightData[i] = graphicsHelper->createShaderParameters(graphicsInstance, goochModelDescLayout, { 1, 2 });
        lightData[i]->setResourceName(
            TCHAR("Light_") + String::toString(i * ARRAY_LENGTH(GoochModelLightArray::lights)) + TCHAR("to") + String::toString(i * ARRAY_LENGTH(GoochModelLightArray::lights) + ARRAY_LENGTH(GoochModelLightArray::lights)));
    }

    const GraphicsResource *drawQuadDescLayout = drawQuadPipelineContext.getPipeline()->getParamLayoutAtSet(0);
    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        const String iString = String::toString(i);
        lightTextures.set(graphicsHelper->createShaderParameters(graphicsInstance, goochModelDescLayout, { 1, 3 }), i);
        lightTextures.getResources()[i]->setResourceName(TCHAR("LightFrameCommon_") + iString);
        drawQuadTextureDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadTextureDescs.getResources()[i]->setResourceName(TCHAR("QuadUnlit_") + iString);
        drawQuadNormalDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadNormalDescs.getResources()[i]->setResourceName(TCHAR("QuadNormal_") + iString);
        drawQuadDepthDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawQuadDepthDescs.getResources()[i]->setResourceName(TCHAR("QuadDepth_") + iString);
        drawLitColorsDescs.set(graphicsHelper->createShaderParameters(graphicsInstance, drawQuadDescLayout), i);
        drawLitColorsDescs.getResources()[i]->setResourceName(TCHAR("QuadLit_") + iString);
    }

    clearInfoParams = graphicsHelper->createShaderParameters(graphicsInstance, clearQuadPipelineContext.getPipeline()->getParamLayoutAtSet(0));
    clearInfoParams->setResourceName(TCHAR("ClearInfo"));

    testComputeParams
        = graphicsHelper->createShaderParameters(graphicsInstance, testComputePipelineContext.getPipeline()->getParamLayoutAtSet(0));
    testComputeParams->setResourceName(TCHAR("TestCompute"));

    createTextRenderResources(cmdList, graphicsInstance, graphicsHelper);

    setupShaderParameterParams(graphicsInstance, graphicsHelper);
}

void ExperimentalEngineGoochModel::setupShaderParameterParams(IGraphicsInstance *, const GraphicsHelperAPI *)
{
    ViewData viewData;
    viewData.view = camera.viewMatrix();
    viewData.invView = viewData.view.inverse();
    viewData.projection = camera.projectionMatrix();
    viewData.invProjection = viewData.projection.inverse();
    viewData.w2clip = viewData.projection * viewData.invView;
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
    lightCommon->setIntParam(TCHAR("lightsCount"), uint32(sceneLightData.size()));
    lightCommon->setFloatParam(TCHAR("invLightsCount"), 1.0f / sceneLightData.size());
    lightCommon->init();
    uint32 lightStartIdx = 0;
    for (ShaderParametersRef &light : lightData)
    {
        uint32 rangeIdx = 0;
        for (; rangeIdx < ARRAY_LENGTH(GoochModelLightArray::lights) && (rangeIdx + lightStartIdx) < sceneLightData.size(); ++rangeIdx)
        {
            light->setBuffer(TCHAR("lights"), sceneLightData[rangeIdx + lightStartIdx], rangeIdx);
        }
        light->setIntParam(TCHAR("count"), uint32(rangeIdx));
        light->init();

        lightStartIdx += ARRAY_LENGTH(GoochModelLightArray::lights);
    }

    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    uint32 swapchainCount = windowCanvas->imagesCount();
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
        lightTextures.getResources()[i]->setTextureParam(TCHAR("ssDepth"), multibufferRts[(3 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParamViewInfo(TCHAR("ssDepth"), depthImageViewInfo);
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssColor"), frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering
        );

        drawQuadTextureDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(0 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadNormalDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(1 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadDepthDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(3 * fbIncrement)], linearFiltering);
        drawQuadDepthDescs.getResources()[i]->setTextureParamViewInfo(TCHAR("quadTexture"), depthImageViewInfo);
        drawLitColorsDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), frameResources[i].lightingPassRt->getTextureResource(), linearFiltering
        );
    }
    lightTextures.init();
    drawQuadTextureDescs.init();
    drawQuadNormalDescs.init();
    drawQuadDepthDescs.init();
    drawLitColorsDescs.init();

    clearInfoParams->setVector4Param(TCHAR("clearColor"), Vector4D(0, 0, 0, 0));
    clearInfoParams->init();

    testComputeParams->setTextureParam(TCHAR("resultImage"), writeTexture->getTextureResource());

    testComputeParams->resizeRuntimeBuffer(TCHAR("inData"), 2);
    auto textures = assetManager.getAssetsOfType<EAssetType::Texture2D, TextureAsset>();
    texturesCount = uint32(textures.size());
    for (uint32 i = 0; i < textures.size(); ++i)
    {
        testComputeParams->setTextureParam(TCHAR("srcImages"), textures[i]->getTexture()->getTextureResource(), linearFiltering, i);
    }
    AOS testRuntime;
    testRuntime.a = Vector4D(1, 0, 1, 0);
    testRuntime.b = Vector2D::FWD;
    testRuntime.c[0] = Vector2D::RIGHT;
    testRuntime.c[1] = Vector2D::FWD;
    testRuntime.c[2] = Vector2D::RIGHT;
    testRuntime.c[3] = Vector2D::FWD;
    testComputeParams->setVector4Param(TCHAR("test1"), testRuntime.a);
    testComputeParams->setBuffer(TCHAR("data"), testRuntime, 0);
    testComputeParams->setBuffer(TCHAR("data"), testRuntime, 1);
    testComputeParams->init();
}

void ExperimentalEngineGoochModel::reupdateTextureParamsOnResize()
{
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    uint32 swapchainCount = windowCanvas->imagesCount();

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
        lightTextures.getResources()[i]->setTextureParam(TCHAR("ssDepth"), multibufferRts[(3 * fbIncrement)], nearestFiltering);
        lightTextures.getResources()[i]->setTextureParam(
            TCHAR("ssColor"), frameResources[i].lightingPassResolved->getTextureResource(), nearestFiltering
        );

        drawQuadTextureDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(0 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadNormalDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), multibufferRts[(1 * fbIncrement) + resolveIdxOffset], linearFiltering
        );
        drawQuadDepthDescs.getResources()[i]->setTextureParam(TCHAR("quadTexture"), multibufferRts[(3 * fbIncrement)], linearFiltering);
        drawLitColorsDescs.getResources()[i]->setTextureParam(
            TCHAR("quadTexture"), frameResources[i].lightingPassRt->getTextureResource(), linearFiltering
        );
    }
}

void ExperimentalEngineGoochModel::destroyShaderParameters()
{
    viewParameters->release();
    viewParameters.reset();

    instanceParameters->release();
    instanceParameters.reset();

    for (auto &shaderUniqParams : sceneShaderUniqParams)
    {
        shaderUniqParams.second->release();
    }
    sceneShaderUniqParams.clear();

    lightCommon->release();
    lightCommon.reset();

    for (ShaderParametersRef &light : lightData)
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

    destroyTextRenderResources();
}

void ExperimentalEngineGoochModel::resizeLightingRts(const Size2D &size)
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
    }
}

void ExperimentalEngineGoochModel::createFrameResources(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
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
        String name = TCHAR("Frame");
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

void ExperimentalEngineGoochModel::destroyFrameResources()
{
    for (int32 i = 0; i < frameResources.size(); ++i)
    {
        frameResources[i].usageWaitSemaphore[0]->release();
        frameResources[i].recordingFence->release();
        frameResources[i].usageWaitSemaphore[0].reset();
        frameResources[i].recordingFence.reset();

        const IRenderTargetTexture *rtPtr = frameResources[i].lightingPassRt;
        rendererModule->getRenderManager()->clearExternInitRtsFramebuffer({ &rtPtr, 1 });
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassRt);
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].lightingPassResolved);
    }
}

void ExperimentalEngineGoochModel::getPipelineForSubpass()
{
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    auto multibufferRts = GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, 0);

    drawSmPipelineContext.forVertexType = EVertexType::StaticMesh;
    drawSmPipelineContext.materialName = TCHAR("SingleColor");
    drawSmPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffer;
    drawSmPipelineContext.swapchainIdx = 0;
    rendererModule->getRenderManager()->preparePipelineContext(&drawSmPipelineContext, multibufferRts);

    // Gooch model
    drawGoochPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawGoochPipelineContext.materialName = TCHAR("GoochModel");
    const IRenderTargetTexture *rtPtr = frameResources[0].lightingPassRt;
    rendererModule->getRenderManager()->preparePipelineContext(&drawGoochPipelineContext, { &rtPtr, 1 });

    clearQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    clearQuadPipelineContext.materialName = TCHAR("ClearRT");
    rtPtr = frameResources[0].lightingPassResolved;
    rendererModule->getRenderManager()->preparePipelineContext(&clearQuadPipelineContext, { &rtPtr, 1 });

    resolveLightRtPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    resolveLightRtPipelineContext.materialName = TCHAR("DrawQuadFromTexture");
    rtPtr = frameResources[0].lightingPassResolved;
    rendererModule->getRenderManager()->preparePipelineContext(&resolveLightRtPipelineContext, { &rtPtr, 1 });

    drawQuadPipelineContext.windowCanvas = windowCanvas;
    drawQuadPipelineContext.materialName = TCHAR("DrawQuadFromTexture");
    drawQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawQuadPipelineContext.swapchainIdx = 0;
    rendererModule->getRenderManager()->preparePipelineContext(&drawQuadPipelineContext);

    testComputePipelineContext.materialName = TCHAR("TestCompute");
    rendererModule->getRenderManager()->preparePipelineContext(&testComputePipelineContext);

    imguiShaderCntxt.materialName = TCHAR("DrawImGui");
    imguiShaderCntxt.renderpassFormat = ERenderPassFormat::Generic;
    rtPtr = frameResources[0].lightingPassRt;
    rendererModule->getRenderManager()->preparePipelineContext(&imguiShaderCntxt, { &rtPtr, 1 });
}

void ExperimentalEngineGoochModel::clearPipelineContexts()
{
    drawSmPipelineContext.reset();
    drawGoochPipelineContext.reset();
    clearQuadPipelineContext.reset();
    resolveLightRtPipelineContext.reset();
    drawQuadPipelineContext.reset();

    testComputePipelineContext.reset();
    imguiShaderCntxt.reset();
}

void ExperimentalEngineGoochModel::createPipelineResources(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    clearValues.colors.resize(drawSmPipelineContext.getFb()->textures.size(), LinearColorConst::BLACK);

    // Shader pipeline's buffers and image access
    createShaderParameters(cmdList, graphicsInstance, graphicsHelper);
}

void ExperimentalEngineGoochModel::destroyPipelineResources()
{
    // Shader pipeline's buffers and image access
    destroyShaderParameters();
}

void ExperimentalEngineGoochModel::createTextRenderResources(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    FontManager &fontManager = *application->fontManager;
    FontManager::FontIndex idx
        = fontManager.addFont(PathFunctions::combinePath(Paths::engineRuntimeRoot(), TCHAR("Assets/Fonts/CascadiaMono-Bold.ttf")));

    fontManager.addGlyphs(
        idx,
        {
            {0x0020u, 0x0100u},
            {     0u,      1u}
    },
        { 32, 64 }
    );
    fontManager.flushUpdates();
    textToRender = TCHAR("Hello World!\nCheck this out!");

    textRenderParams = graphicsHelper->createShaderParameters(graphicsInstance, imguiShaderCntxt.getPipeline()->getParamLayoutAtSet(0));
    textRenderParams->setResourceName(TCHAR("TestTextRenderParams"));
    // Just tiny hack now
    fontManager.setupTextureAtlas(textRenderParams.get(), TCHAR("textureAtlas"));
    textRenderParams->setTextureParam(TCHAR("textureAtlas"), textRenderParams->getTextureParam(TCHAR("textureAtlas")), linearFiltering);
    textRenderParams->init();

    textVertsBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, int32(sizeof(ImDrawVert)));
    textVertsBuffer->setResourceName(TCHAR("TestTextRenderVertices"));
    textIndexBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, int32(sizeof(ImDrawIdx)));
    textIndexBuffer->setResourceName(TCHAR("TestTextRenderIndices"));
    updateTextRenderData(cmdList, graphicsInstance, graphicsHelper);
}

void ExperimentalEngineGoochModel::destroyTextRenderResources()
{
    textRenderParams.reset();
    textVertsBuffer.reset();
    textIndexBuffer.reset();
}

void ExperimentalEngineGoochModel::updateTextRenderData(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    FontManager &fontManager = *application->fontManager;
    std::vector<FontVertex> fontVerts;
    fontManager.draw(fontVerts, textBB, textToRender, 0, textHeight, wrapSize);
    Color fontCol{ textCol };

    if (fontVerts.size() > textVertsBuffer->bufferCount())
    {
        // Recreate and resize vertex and index buffer
        if (textIndexBuffer->isValid())
        {
            textIndexBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, int32(sizeof(uint32)));
            textIndexBuffer->setResourceName(TCHAR("TestTextRenderIndices"));
        }
        if (textVertsBuffer->isValid())
        {
            textVertsBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, int32(sizeof(ImDrawVert)));
            textVertsBuffer->setResourceName(TCHAR("TestTextRenderVertices"));
        }
        textVertsBuffer->setBufferCount(uint32(fontVerts.size()));
        textVertsBuffer->init();
        textIndexBuffer->setBufferCount(uint32(fontVerts.size() * 6) / 4);
        textIndexBuffer->init();
    }

    textVertCount = uint32(fontVerts.size());
    textIdxCount = (textVertCount * 6) / 4;
    // Copying directly as we can use transform to move the vertices in place
    std::vector<ImDrawVert> verts(textVertCount);
    std::vector<uint32> idxs(textIdxCount);
    uint32 endIdx = uint32(verts.size() / 4);
    for (uint32 idx = 0; idx < endIdx; ++idx)
    {
        uint32 vertIdx = idx * 4;
        for (uint32 i = 0; i < 4; ++i)
        {
            ImDrawVert &thisVert = verts[vertIdx + i];
            thisVert.pos = { float(fontVerts[vertIdx + i].pos.x), float(fontVerts[vertIdx + i].pos.y) };
            thisVert.uv = fontVerts[vertIdx + i].texCoord;
            thisVert.col = fontCol;
        }
        uint32 indexIdx = idx * 6;
        idxs[indexIdx + 0] = vertIdx + 0;
        idxs[indexIdx + 1] = vertIdx + 2;
        idxs[indexIdx + 2] = vertIdx + 3;
        idxs[indexIdx + 3] = vertIdx + 0;
        idxs[indexIdx + 4] = vertIdx + 1;
        idxs[indexIdx + 5] = vertIdx + 2;
    }
    std::vector<BatchCopyBufferData> copies(2);
    copies[0] = BatchCopyBufferData{ textIndexBuffer, 0, idxs.data(), uint32(idxs.size() * sizeof(uint32)) };
    copies[1] = BatchCopyBufferData{ textVertsBuffer, 0, verts.data(), uint32(verts.size() * sizeof(ImDrawVert)) };
    cmdList->copyToBuffer(copies);

    Int2D textSize = textBB.size();
    Vector2D scale = 2.0f / Vector2D(float(textSize.x), float(textSize.y));
    Vector2D translate{ -1.0f - Vector2D(textBB.minBound) * scale };
    // Now offset transforms so that text will be centered in middle of screen
    textBB += (Int2D(ApplicationSettings::screenSize.get()) - textSize) / 2;

    textRenderParams->setVector2Param(TCHAR("scale"), scale);
    textRenderParams->setVector2Param(TCHAR("translate"), translate);
}

void ExperimentalEngineGoochModel::updateCameraParams()
{
    ViewData viewDataTemp;

    if (application->inputSystem->isKeyPressed(Keys::RMB))
    {
        cameraRotation.yaw()
            += application->inputSystem->analogState(AnalogStates::RelMouseX)->currentValue * timeData.activeTimeDilation * 0.25f;
        cameraRotation.pitch()
            += application->inputSystem->analogState(AnalogStates::RelMouseY)->currentValue * timeData.activeTimeDilation * 0.25f;
    }

    if (application->inputSystem->isKeyPressed(Keys::A))
    {
        cameraTranslation -= cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::D))
    {
        cameraTranslation += cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::W))
    {
        cameraTranslation += cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::S))
    {
        cameraTranslation -= cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::Q))
    {
        cameraTranslation -= Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::E))
    {
        cameraTranslation += Vector3D::UP * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (application->inputSystem->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = RotationMatrix::fromZX(Vector3D::UP, cameraRotation.fwdVector()).asRotation();
    }

    if (camera.cameraProjection != projection)
    {
        camera.cameraProjection = projection;
        viewDataTemp.projection = camera.projectionMatrix();
        viewDataTemp.invProjection = viewDataTemp.projection.inverse();
        viewDataTemp.w2clip = viewDataTemp.projection * camera.viewMatrix().inverse();

        viewParameters->setMatrixParam(TCHAR("projection"), viewDataTemp.projection);
        viewParameters->setMatrixParam(TCHAR("invProjection"), viewDataTemp.invProjection);
        viewParameters->setMatrixParam(TCHAR("w2clip"), viewDataTemp.w2clip);
        lightCommon->setMatrixParam(TCHAR("projection"), viewDataTemp.projection);
        lightCommon->setMatrixParam(TCHAR("invProjection"), viewDataTemp.invProjection);
        lightCommon->setMatrixParam(TCHAR("w2clip"), viewDataTemp.w2clip);
    }

    camera.setRotation(cameraRotation);
    camera.setTranslation(cameraTranslation);

    viewDataTemp.view = camera.viewMatrix();
    viewDataTemp.invView = viewDataTemp.view.inverse();
    viewDataTemp.w2clip = camera.projectionMatrix() * viewDataTemp.invView;

    viewParameters->setMatrixParam(TCHAR("view"), viewDataTemp.view);
    viewParameters->setMatrixParam(TCHAR("invView"), viewDataTemp.invView);
    viewParameters->setMatrixParam(TCHAR("w2clip"), viewDataTemp.w2clip);
    lightCommon->setMatrixParam(TCHAR("view"), viewDataTemp.view);
    lightCommon->setMatrixParam(TCHAR("invView"), viewDataTemp.invView);
    lightCommon->setMatrixParam(TCHAR("w2clip"), viewDataTemp.w2clip);
}

void ExperimentalEngineGoochModel::onStartUp()
{
    TestGameEngine::onStartUp();

    ENQUEUE_COMMAND(EngineStartUp)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            createSceneRenderData(cmdList, graphicsInstance, graphicsHelper);
            startUpRenderInit(cmdList, graphicsInstance, graphicsHelper);
        }
    );

    camera.cameraProjection = projection;
    camera.setOrthoSize({ 1280, 720 });
    camera.setClippingPlane(0.1f, 6000.f);
    camera.setFOV(110.f, 90.f);

    cameraTranslation = Vector3D(0.f, -1.f, 0.0f).safeNormalized() * (500);
    cameraTranslation.z() += 200;

    camera.setTranslation(cameraTranslation);
    camera.lookAt(Vector3D::ZERO);
    cameraRotation = camera.rotation();

    getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(shared_from_this()));
    createScene();

    std::vector<ShortSizeBox2D> bxs{
        ShortSizeBox2D{ShortSize2D{ 0 },  ShortSize2D{ 64, 80 }},
        ShortSizeBox2D{ShortSize2D{ 0 }, ShortSize2D{ 128, 48 }},
        ShortSizeBox2D{ShortSize2D{ 0 },  ShortSize2D{ 48, 56 }},
        ShortSizeBox2D{ShortSize2D{ 0 }, ShortSize2D{ 64, 128 }},
        ShortSizeBox2D{ShortSize2D{ 0 },      ShortSize2D{ 32 }},
        ShortSizeBox2D{ShortSize2D{ 0 },  ShortSize2D{ 61, 35 }},
        ShortSizeBox2D{ShortSize2D{ 0 },  ShortSize2D{ 45, 51 }},
        ShortSizeBox2D{ShortSize2D{ 0 },  ShortSize2D{ 33, 37 }},
        ShortSizeBox2D{ShortSize2D{ 0 },  ShortSize2D{ 70, 21 }}
    };
    boxes.swap(bxs);
    std::vector<ShortSizeBox2D *> boxPtrs{ boxes.data(),     boxes.data() + 1, boxes.data() + 2, boxes.data() + 3, boxes.data() + 4,
                                           boxes.data() + 5, boxes.data() + 6, boxes.data() + 7, boxes.data() + 8 };
    std::vector<PackedRectsBin<ShortSizeBox2D>> packedbins;
    ShortSize2D binSize{ 256 };
    bool bPacked = MathGeom::packRectangles(packedbins, binSize, boxPtrs);
    if (!bPacked)
    {
        LOG_ERROR("ExperimentalEngineGoochModel", "Failed packing rectangles!");
    }

    tempTest();
}

void ExperimentalEngineGoochModel::startUpRenderInit(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    frameResources.resize(windowCanvas->imagesCount());
    GBuffers::initialize(windowCanvas->imagesCount());

    createFrameResources(graphicsInstance, graphicsHelper);
    getPipelineForSubpass();
    createImages(graphicsInstance, graphicsHelper);
    createPipelineResources(cmdList, graphicsInstance, graphicsHelper);
    createDrawCmdsBuffer(graphicsInstance, graphicsHelper);
}

void ExperimentalEngineGoochModel::onQuit()
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
    TestGameEngine::onQuit();
}

void ExperimentalEngineGoochModel::renderQuit()
{
    GBuffers::destroy();

    clearPipelineContexts();
    destroyDrawCmdsBuffer();
    destroyPipelineResources();
    destroyFrameResources();

    destroyImages();

    destroyScene();
}

void ExperimentalEngineGoochModel::frameRender(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    WindowCanvasRef windowCanvas = application->windowManager->getWindowCanvas(application->windowManager->getMainWindow());
    SemaphoreRef waitSemaphore;
    uint32 index = windowCanvas->requestNextImage(&waitSemaphore, nullptr);
    drawQuadPipelineContext.swapchainIdx = index;
    rendererModule->getRenderManager()->preparePipelineContext(&drawQuadPipelineContext);

    auto gbufferRts = GBuffers::getGbufferRts(ERenderPassFormat::Multibuffer, index);
    rendererModule->getRenderManager()->preparePipelineContext(&drawSmPipelineContext, gbufferRts);

    const IRenderTargetTexture *rtPtr = frameResources[index].lightingPassRt;
    rendererModule->getRenderManager()->preparePipelineContext(&drawGoochPipelineContext, { &rtPtr, 1 });
    rtPtr = frameResources[index].lightingPassResolved;
    rendererModule->getRenderManager()->preparePipelineContext(&resolveLightRtPipelineContext, { &rtPtr, 1 });

    GraphicsPipelineQueryParams queryParam;
    queryParam.cullingMode = ECullingMode::BackFace;
    queryParam.drawMode = EPolygonDrawMode::Fill;

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

    if (bCompute)
    {
        if (bAnimateX || bAnimateY)
        {
            timeAccum += timeData.deltaTime;
            testBindlessTextureIdx += uint32(Math::floor(timeAccum / 2.0f));
            testBindlessTextureIdx %= texturesCount;
            timeAccum = Math::mod(timeAccum, 2.0f);
        }
        std::vector<std::pair<String, std::any>> computePushConsts = {
            {    TCHAR("time"),                                    { Time::asSeconds(Time::timeNow()) }},
              {   TCHAR("flags"), { uint32((bAnimateX ? 0x00000001 : 0) | (bAnimateY ? 0x00000010 : 0)) }},
                {TCHAR("srcIndex"),                                              { testBindlessTextureIdx }}
        };
        cmdList->finishCmd(cmdName + "_Comp");
        auto temp = cmdList->startCmd(cmdName + "_Comp", EQueueFunction::Compute, true);
        cmdList->cmdBindComputePipeline(temp, testComputePipelineContext);
        cmdList->cmdPushConstants(temp, testComputePipelineContext, computePushConsts);
        cmdList->cmdBindDescriptorsSets(temp, testComputePipelineContext, testComputeParams);
        cmdList->cmdBarrierResources(temp, { &testComputeParams, 1 });
        cmdList->cmdDispatch(
            temp,
            writeTexture->getTextureSize().x
                / static_cast<const ComputeShaderConfig *>(testComputePipelineContext.getPipeline()->getShaderResource()->getShaderConfig())
                      ->getSubGroupSize()
                      .x,
            writeTexture->getTextureSize().y
                / static_cast<const ComputeShaderConfig *>(testComputePipelineContext.getPipeline()->getShaderResource()->getShaderConfig())
                      ->getSubGroupSize()
                      .y
        );
        cmdList->cmdReleaseQueueResources(temp, EQueueFunction::Graphics);
        cmdList->endCmd(temp);

        CommandSubmitInfo2 cs2;
        cs2.cmdBuffers = { temp };
        cmdList->submitCmd(EQueuePriority::High, cs2);
    }
    const GraphicsResource *cmdBuffer = cmdList->startCmd(cmdName, EQueueFunction::Graphics, true);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ExperimentalEngineFrame);
        // cmdList->cmdBindComputePipeline(cmdBuffer, testComputePipelineContext);
        // cmdList->cmdPushConstants(cmdBuffer, testComputePipelineContext, computerPushConsts);
        // cmdList->cmdBindDescriptorsSets(cmdBuffer, testComputePipelineContext, testComputeParams);
        // cmdList->cmdBarrierResources(cmdBuffer, { &testComputeParams, 1 });
        // cmdList->cmdDispatch(
        //     cmdBuffer,
        //     writeTexture->getTextureSize().x
        //         / static_cast<const ComputeShaderConfig *>(testComputePipelineContext.getPipeline()->getShaderResource()->getShaderConfig())
        //               ->getSubGroupSize()
        //               .x,
        //     writeTexture->getTextureSize().y
        //         / static_cast<const ComputeShaderConfig *>(testComputePipelineContext.getPipeline()->getShaderResource()->getShaderConfig())
        //               ->getSubGroupSize()
        //               .y
        //);

        cmdList->cmdBeginRenderPass(cmdBuffer, drawSmPipelineContext, scissor, {}, clearValues);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, MainUnlitPass);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
            cmdList->cmdBindVertexBuffer(cmdBuffer, 0, sceneVertexBuffer, 0);
            cmdList->cmdBindIndexBuffer(cmdBuffer, sceneIndexBuffer);
            for (const auto &pipelineToOffsetCount : pipelineToDrawCmdOffsetCount)
            {
                cmdList->cmdBindGraphicsPipeline(cmdBuffer, *pipelineToOffsetCount.first, { queryParam });

                // Shader material params set
                ShaderParametersRef descSets[]
                    = { viewParameters.get(), instanceParameters.get(), sceneShaderUniqParams[pipelineToOffsetCount.first].get() };
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
                cmdList->cmdBindDescriptorsSets(cmdBuffer, clearQuadPipelineContext, clearInfoParams.get());
                cmdList->cmdDrawVertices(cmdBuffer, 0, 3);
            }
            cmdList->cmdEndRenderPass(cmdBuffer);

            int32 lightDataIndex = 0;
            for (const ShaderParametersRef &light : lightData)
            {
                cmdList->cmdBeginRenderPass(cmdBuffer, drawGoochPipelineContext, scissor, {}, clearValues);
                {
                    SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawLight);
                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawGoochPipelineContext, { queryParam });

                    ShaderParametersRef descSets[] = { lightCommon.get(), *lightTextures, light.get() };
                    cmdList->cmdBindDescriptorsSets(cmdBuffer, drawGoochPipelineContext, descSets);
                    cmdList->cmdDrawVertices(cmdBuffer, 0, 3);
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

                        cmdList->cmdDrawVertices(cmdBuffer, 0, 3);
                    }
                    cmdList->cmdEndRenderPass(cmdBuffer);
                }
            }
        }
        else
        {
            ShaderParametersRef drawQuadDescs = nullptr;
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

            if (drawQuadDescs.isValid())
            {
                rtPtr = frameResources[index].lightingPassRt;
                rendererModule->getRenderManager()->preparePipelineContext(&resolveLightRtPipelineContext, { &rtPtr, 1 });

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
        ImGuiDrawingContext drawingContext;
        drawingContext.cmdBuffer = cmdBuffer;
        drawingContext.rtTexture = frameResources[index].lightingPassRt;
        getImGuiManager().draw(cmdList, graphicsInstance, graphicsHelper, drawingContext);
        cmdList->cmdReleaseQueueResources(cmdBuffer, EQueueFunction::Compute);

        // Drawing text
        rtPtr = frameResources[index].lightingPassRt;
        rendererModule->getRenderManager()->preparePipelineContext(&imguiShaderCntxt, { &rtPtr, 1 });
        if (!textToRender.empty())
        {
            RenderPassAdditionalProps additionalProps;
            additionalProps.bAllowUndefinedLayout = false;
            additionalProps.colorAttachmentLoadOp = EAttachmentOp::LoadOp::Load;
            additionalProps.depthLoadOp = EAttachmentOp::LoadOp::Load;
            additionalProps.stencilLoadOp = EAttachmentOp::LoadOp::Load;

            QuantizedBox2D textScissor = QuantizedBox2D(
                Math::clamp(textBB.minBound, Int2D(0), frameResources[index].lightingPassRt->getTextureSize()),
                Math::clamp(textBB.maxBound, Int2D(0), frameResources[index].lightingPassRt->getTextureSize())
            );
            cmdList->cmdSetViewportAndScissor(cmdBuffer, textBB, textScissor);
            cmdList->cmdBeginRenderPass(cmdBuffer, imguiShaderCntxt, textScissor, additionalProps, clearValues);
            {
                SCOPED_CMD_MARKER(cmdList, cmdBuffer, TextRender);

                cmdList->cmdBindGraphicsPipeline(cmdBuffer, imguiShaderCntxt, { queryParam });
                cmdList->cmdBindDescriptorsSets(cmdBuffer, imguiShaderCntxt, textRenderParams);
                cmdList->cmdBindVertexBuffer(cmdBuffer, 0, textVertsBuffer, 0);
                cmdList->cmdBindIndexBuffer(cmdBuffer, textIndexBuffer);
                cmdList->cmdDrawIndexed(cmdBuffer, 0, textIdxCount);
            }
            cmdList->cmdEndRenderPass(cmdBuffer);
        }

        // Drawing final quad
        viewport.maxBound = scissor.maxBound = ApplicationSettings::surfaceSize.get();

        cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);

        RenderPassAdditionalProps renderPassAdditionalProps;
        renderPassAdditionalProps.bUsedAsPresentSource = true;
        cmdList->cmdBeginRenderPass(cmdBuffer, drawQuadPipelineContext, scissor, renderPassAdditionalProps, clearValues);
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveToSwapchain);

            cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
            cmdList->cmdBindGraphicsPipeline(cmdBuffer, drawQuadPipelineContext, { queryParam });
            cmdList->cmdBindDescriptorsSets(cmdBuffer, drawQuadPipelineContext, *drawLitColorsDescs);
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
        {frameResources[index].usageWaitSemaphore[0], EPipelineStages::ColorAttachmentOutput}
    };
    submitInfo.cmdBuffers = { cmdBuffer };

    cmdList->submitCmd(EQueuePriority::High, submitInfo, frameResources[index].recordingFence);

    std::vector<WindowCanvasRef> canvases = { windowCanvas };
    std::vector<uint32> indices = { index };
    cmdList->presentImage(canvases, indices, { frameResources[index].usageWaitSemaphore });
}

void ExperimentalEngineGoochModel::tickEngine()
{
    TestGameEngine::tickEngine();
    updateCameraParams();

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

    if (!application->windowManager->getMainWindow()->isMinimized())
    {
        ENQUEUE_COMMAND(TickFrame)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                frameRender(cmdList, graphicsInstance, graphicsHelper);
            }
        );
    }

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

int32 ExperimentalEngineGoochModel::layerDepth() const { return 0; }

int32 ExperimentalEngineGoochModel::sublayerDepth() const { return 0; }

void ExperimentalEngineGoochModel::draw(class ImGuiDrawInterface *drawInterface)
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

            const InputAnalogState *rmxState = application->inputSystem->analogState(AnalogStates::RelMouseX);
            const InputAnalogState *rmyState = application->inputSystem->analogState(AnalogStates::RelMouseY);
            const InputAnalogState *amxState = application->inputSystem->analogState(AnalogStates::AbsMouseX);
            const InputAnalogState *amyState = application->inputSystem->analogState(AnalogStates::AbsMouseY);
            ImGui::Text(
                "Cursor pos (%.0f, %.0f) Delta (%0.1f, %0.1f)", amxState->currentValue, amyState->currentValue, rmxState->currentValue,
                rmyState->currentValue
            );

            if (ImGui::CollapsingHeader("Camera"))
            {
                ImGui::Columns(2);
                ImGui::Text("Projection");
                ImGui::NextColumn();
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
            }

            ImGui::Columns(1);
            ImGui::NextColumn();
            if (ImGui::CollapsingHeader("Rendering"))
            {
                ImGui::Columns(2);
                ImGui::Text("Render Size");
                ImGui::NextColumn();
                {
                    const char *resolutions[] = { "1280x720", "1920x1080", "2560x1440", "3840x2160" };
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
                    const char *bufferMode[] = { "Lit", "Unlit", "Normal", "Depth" };
                    ImGui::Combo("Frame", &frameVisualizeId, bufferMode, ARRAY_LENGTH(bufferMode));
                }
            };

            ImGui::Columns(1);
            ImGui::NextColumn();
            if (ImGui::CollapsingHeader("Compute"))
            {
                bCompute = true;
                ImGui::Text("Animate");
                ImGui::NextColumn();
                ImGui::Checkbox("X", &bAnimateX);
                ImGui::SameLine();
                ImGui::Checkbox("Y", &bAnimateY);
                ImGui::NextColumn();
                ImGui::Text("%f", Time::asSeconds(Time::timeNow()));

                ImGui::Separator();
                ImGui::NextColumn();
                ImGui::Image(
                    writeTexture->getTextureResource().get(), ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowContentRegionWidth())
                );
                ImGui::Separator();
            }
            else
            {
                bCompute = false;
            }

            if (ImGui::CollapsingHeader("Bitonic Sort"))
            {
                static TestBitonicSortIndices bitonic(16);
                if (ImGui::InputInt("Count", &bitonic.count))
                {
                    bitonic = TestBitonicSortIndices(bitonic.count);
                }

                if (ImPlot::BeginPlot("Bitonic Threads", ImVec2(-1, 0), ImPlotFlags_::ImPlotFlags_CanvasOnly))
                {
                    ImPlot::SetupAxis(
                        ImAxis_X1, nullptr, ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_NoGridLines
                    );
                    ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_Invert);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0, bitonic.stepsCount + 1, ImGuiCond_::ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, bitonic.count, ImGuiCond_::ImGuiCond_Always);

                    int32 idx = 0;
                    for (const auto &threadInds : bitonic.perThreadIndices)
                    {
                        String labelId = TCHAR("Thread: ") + String::toString(idx);
                        ImPlot::PushStyleColor(ImPlotCol_::ImPlotCol_Line, LinearColor(threadInds.second));
                        int32 segIdx = 0;
                        for (const TestBitonicSortIndices::LineSegment &seg : threadInds.first)
                        {
                            String segId = labelId + "Segment : " + String::toString(segIdx);
                            ImPlot::PlotLine(segId.getChar(), seg.step.data(), seg.indices.data(), int32(seg.indices.size()));
                            segIdx++;
                        }
                        ImPlot::PopStyleColor();
                        idx++;
                    }
                    ImPlot::EndPlot();
                }

                if (ImPlot::BeginPlot("Bitonic Groups", ImVec2(-1, 0), ImPlotFlags_::ImPlotFlags_CanvasOnly))
                {
                    ImPlot::SetupAxis(
                        ImAxis_X1, nullptr, ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_NoGridLines
                    );
                    ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_::ImPlotAxisFlags_Lock | ImPlotAxisFlags_::ImPlotAxisFlags_Invert);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0, bitonic.stepsCount + 1, ImGuiCond_::ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, bitonic.count, ImGuiCond_::ImGuiCond_Always);

                    int32 idx = 0;
                    for (const auto &grpInds : bitonic.perGroup)
                    {
                        String labelId = TCHAR("Group: ") + String::toString(idx);
                        ImPlot::PushStyleColor(ImPlotCol_::ImPlotCol_Line, LinearColor(grpInds.second));
                        int32 segIdx = 0;
                        for (const TestBitonicSortIndices::LineSegment &seg : grpInds.first)
                        {
                            String segId = labelId + "Segment : " + String::toString(segIdx);
                            ImPlot::PlotLine(segId.getChar(), seg.step.data(), seg.indices.data(), int32(seg.indices.size()));
                            segIdx++;
                        }
                        ImPlot::PopStyleColor();
                        idx++;
                    }

                    ImPlot::EndPlot();
                }
            }

            if (ImGui::CollapsingHeader("Rect Packer"))
            {
                static std::vector<Color> col;
                if (boxes.size() > col.size())
                {
                    uint32 i = uint32(col.size());
                    col.resize(boxes.size());
                    for (; i < col.size(); ++i)
                    {
                        col[i] = ColorConst::random(127);
                    }
                }
                drawInterface->drawPackedRectangles(
                    boxes.data(), col.data(), (uint32)(boxes.size()), ShortSizeBox2D::PointType(256), ColorConst::RED
                );
            }

            if (ImGui::CollapsingHeader("Font Manager Test"))
            {
                bool bAnyModified = false;
                if (drawInterface->inputTextMultiline(
                        "Text", &textToRender, ImVec2(ImGui::GetWindowContentRegionWidth(), 200),
                        /*ImGuiInputTextFlags_EnterReturnsTrue | */
                        ImGuiInputTextFlags_CtrlEnterForNewLine
                    ))
                {
                    bAnyModified = true;
                }
                if (ImGui::ColorEdit4("Color", reinterpret_cast<float *>(&textCol)))
                {
                    bAnyModified = true;
                }
                if (ImGui::InputInt("Wrap size", &wrapSize, 16))
                {
                    bAnyModified = true;
                }
                const char *fontHeightOpts[] = { "32", "64" };
                static int32 fontOpt = 0;
                if (ImGui::Combo("Font height", &fontOpt, fontHeightOpts, ARRAY_LENGTH(fontHeightOpts)))
                {
                    bAnyModified = true;
                    switch (fontOpt)
                    {
                    case 1:
                        textHeight = 64;
                        break;
                    case 0:
                    default:
                        textHeight = 32;
                        break;
                    }
                }
                if (bAnyModified)
                {
                    ENQUEUE_COMMAND(UpdateTextData)
                    (
                        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
                        {
                            updateTextRenderData(cmdList, graphicsInstance, graphicsHelper);
                        }
                    );
                }
            }

            ImGui::PopStyleVar();
            ImGui::End();
        }
    }
}

TestGameEngine *getExperimentalEngineGoochModel()
{
    static SharedPtr<ExperimentalEngineGoochModel> engineInst = std::make_shared<ExperimentalEngineGoochModel>();
    return engineInst.get();
}
#endif