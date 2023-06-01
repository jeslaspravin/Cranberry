/*!
 * \file EngineRenderScene.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EngineRenderScene.h"
#include "Math/Camera.h"
#include "Math/Plane.h"
#include "IApplicationModule.h"
#include "ApplicationInstance.h"
#include "Classes/World.h"
#include "Classes/Actor.h"
#include "CBEObjectHelpers.h"
#include "Components/RenderableComponent.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
#include "Types/Platform/Threading/CoPaT/CoroutineAwaitAll.h"
#include "Types/Platform/Threading/CoPaT/DispatchHelpers.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderApi/Scene/RenderScene.h"
#include "IRenderInterfaceModule.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"

#define DISABLE_PER_FRAME_UPDATE 0

namespace ERendererIntermTexture
{

static_assert(ERendererIntermTexture::MaxCount == 5, "Update added/removed ERendererIntermTexture format");

EPixelDataFormat::Type getPixelFormat(Type textureType)
{
    switch (textureType)
    {
    case ERendererIntermTexture::GBufferDiffuse:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[0];
        break;
    case ERendererIntermTexture::GBufferNormal:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[1];
        break;
    case ERendererIntermTexture::GBufferARM:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[2];
        break;
    case ERendererIntermTexture::GBufferDepth:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[3];
        break;
    case ERendererIntermTexture::FinalColor:
        return GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer)[0];
        break;
    default:
        break;
    }
    return EPixelDataFormat::BGRA_U8_Norm;
}

const TChar *toString(Type textureType)
{
    switch (textureType)
    {
    case ERendererIntermTexture::GBufferDiffuse:
        return TCHAR("GBuffer_Diffuse");
        break;
    case ERendererIntermTexture::GBufferNormal:
        return TCHAR("GBuffer_Normal");
        break;
    case ERendererIntermTexture::GBufferARM:
        return TCHAR("GBuffer_ARM");
        break;
    case ERendererIntermTexture::GBufferDepth:
        return TCHAR("GBuffer_Depth");
        break;
    case ERendererIntermTexture::FinalColor:
        return TCHAR("FinalColor");
        break;
    default:
        break;
    }
    return TCHAR("InvalidIntermFormat");
}

} // namespace ERendererIntermTexture

//////////////////////////////////////////////////////////////////////////
// SceneRenderTexturePool Implementations
//////////////////////////////////////////////////////////////////////////

const RendererIntermTexture &
SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt3 size, PoolTextureDesc textureDesc)
{
    ASSERT_INSIDE_RENDERTHREAD();

    if (const RendererIntermTexture *foundTexture = getTexture(cmdList, rtType, size))
    {
        return *foundTexture;
    }

    // If we are not doing MSAA then the MIP count also must be 1. As texture and RT will be same
    const bool bMSAATexture = textureDesc.sampleCount != EPixelSampleCount::SampleCount1;
    debugAssert(bMSAATexture || textureDesc.mipCount == 1);
    ImageResourceCreateInfo ci;
    ci.dimensions = size;
    ci.imageFormat = ERendererIntermTexture::getPixelFormat(rtType);
    ci.numOfMips = 1;
    ci.layerCount = textureDesc.layerCount;

    TextureList::size_type idx = textures.get();
    textures[idx].clearCounter = bufferingCount;
    RendererIntermTexture &texture = textures[idx].intermTexture;
    texture.rtTexture = texture.resolvedTexture
        = IRenderInterfaceModule::get()->currentGraphicsHelper()->createRTImage(IRenderInterfaceModule::get()->currentGraphicsInstance(), ci);
    texture.rtTexture->setResourceName(ERendererIntermTexture::toString(rtType) + String::toString(idx));
    if (bMSAATexture)
    {
        texture.rtTexture->setSampleCounts(textureDesc.sampleCount);

        ci.numOfMips = textureDesc.mipCount;
        texture.resolvedTexture
            = IRenderInterfaceModule::get()->currentGraphicsHelper()->createImage(IRenderInterfaceModule::get()->currentGraphicsInstance(), ci);
        texture.resolvedTexture->setShaderUsage(EImageShaderUsage::Sampling);
        texture.resolvedTexture->setResourceName(ERendererIntermTexture::toString(rtType) + String::toString(idx) + TCHAR("_Resolved"));
        texture.resolvedTexture->init();
    }
    else
    {
        texture.rtTexture->setShaderUsage(EImageShaderUsage::Sampling);
    }
    texture.rtTexture->init();
    LOG_VERBOSE(
        "SceneRenderTexturePool", "Allocated new RT {}({}, {}, {}) under type {}", texture.renderTargetResource()->getResourceName(),
        texture.rtTexture->getImageSize().x, texture.rtTexture->getImageSize().y, texture.rtTexture->getImageSize().z,
        ERendererIntermTexture::toString(rtType)
    );

    // Insert into pool
    poolTextures[rtType].emplace(std::piecewise_construct, std::forward_as_tuple(size), std::forward_as_tuple(idx));
    return texture;
}

const RendererIntermTexture &
SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt2 size, PoolTextureDesc textureDesc)
{
    ASSERT_INSIDE_RENDERTHREAD();
    return getTexture(cmdList, rtType, UInt3(size, 1), textureDesc);
}

const RendererIntermTexture *SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt3 size)
{
    ASSERT_INSIDE_RENDERTHREAD();

    TexturePoolListKey key{ size };

    auto texturesRange = poolTextures[rtType].equal_range(key);
    for (auto itr = texturesRange.first; itr != texturesRange.second; ++itr)
    {
        debugAssert(textures.isValid(itr->second));
        const RendererIntermTexture &intermTexture = textures[itr->second].intermTexture;
        textures[itr->second].clearCounter = bufferingCount;

        // Must be valid if present in poolTextures
        debugAssert(intermTexture.renderTargetResource().isValid());

        if (!cmdList->hasCmdsUsingResource(intermTexture.renderTargetResource(), /*bFinishCmds*/ false)
            && (intermTexture.renderTargetResource() == intermTexture.renderResource()
                || !cmdList->hasCmdsUsingResource(intermTexture.renderResource(), /*bFinishCmds*/ false)))
        {
            return &intermTexture;
        }
    }
    return nullptr;
}

const RendererIntermTexture *SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt2 size)
{
    ASSERT_INSIDE_RENDERTHREAD();
    return getTexture(cmdList, rtType, UInt3(size, 1));
}

void SceneRenderTexturePool::clearUnused(IRenderCommandList *cmdList)
{
    ASSERT_INSIDE_RENDERTHREAD();

    std::vector<ImageResourceRef> safeToDeleteRts;
    safeToDeleteRts.reserve(textures.size());
    for (uint32 i = 0; i != ERendererIntermTexture::MaxCount; ++i)
    {
        for (PoolTexturesMap::iterator itr = poolTextures[i].begin(); itr != poolTextures[i].end();)
        {
            debugAssert(textures.isValid(itr->second));
            TextureData &textureData = textures[itr->second];
            if (textureData.clearCounter != 0)
            {
                textureData.clearCounter--;
                ++itr;
                continue;
            }

            RendererIntermTexture &intermTexture = textureData.intermTexture;
            // Must be valid if present in poolTextures
            debugAssert(intermTexture.renderTargetResource().isValid());

            if (!cmdList->hasCmdsUsingResource(intermTexture.renderTargetResource(), /*bFinishCmds*/ false)
                && (intermTexture.renderTargetResource() == intermTexture.renderResource()
                    || !cmdList->hasCmdsUsingResource(intermTexture.renderResource(), /*bFinishCmds*/ false)))
            {
                safeToDeleteRts.emplace_back(intermTexture.renderTargetResource());
                if (intermTexture.renderTargetResource() != intermTexture.renderResource())
                {
                    safeToDeleteRts.emplace_back(intermTexture.renderResource());
                }

                LOG_VERBOSE(
                    "SceneRenderTexturePool", "Clearing Texture {}({}, {}, {}) from type {}",
                    intermTexture.renderTargetResource()->getResourceName(), intermTexture.rtTexture->getImageSize().x,
                    intermTexture.rtTexture->getImageSize().y, intermTexture.rtTexture->getImageSize().z,
                    ERendererIntermTexture::toString(ERendererIntermTexture::Type(i))
                );
                textures.reset(itr->second);
                itr = poolTextures[i].erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }

    if (!safeToDeleteRts.empty())
    {
        RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
        renderMan->getGlobalRenderingContext()->clearFbsContainingRts(safeToDeleteRts);
    }
}

void SceneRenderTexturePool::clearPool(IRenderCommandList *cmdList)
{
    ASSERT_INSIDE_RENDERTHREAD();

    std::vector<ImageResourceRef> allRts;
    allRts.reserve(2 * textures.size());
    for (const TextureData &textureData : textures)
    {
        if (textureData.intermTexture.renderTargetResource().isValid())
        {
            cmdList->waitOnResDepCmds(textureData.intermTexture.renderTargetResource());
            allRts.emplace_back(textureData.intermTexture.renderTargetResource());
            if (textureData.intermTexture.renderTargetResource() != textureData.intermTexture.renderResource())
            {
                cmdList->waitOnResDepCmds(textureData.intermTexture.renderResource());
                allRts.emplace_back(textureData.intermTexture.renderResource());
            }
        }
    }

    textures.clear();
    for (uint32 i = ERendererIntermTexture::GBufferDiffuse; i != ERendererIntermTexture::MaxCount; ++i)
    {
        poolTextures[i].clear();
    }

    RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
    renderMan->getGlobalRenderingContext()->clearFbsContainingRts(allRts);
}

//////////////////////////////////////////////////////////////////////////
// EngineRenderScene Implementations
//////////////////////////////////////////////////////////////////////////

STRINGID_CONSTEXPR static const StringID MATERIAL_BUFFER_NAME = STRID("materials");
STRINGID_CONSTEXPR static const StringID INSTANCES_BUFFER_NAME = STRID("instancesWrapper");

const RendererIntermTexture &EngineRenderScene::getFinalColor(IRenderCommandList *cmdList, Short2 size)
{
    debugAssert(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get() == EPixelSampleCount::SampleCount1);

    return rtPool.getTexture(cmdList, ERendererIntermTexture::FinalColor, size, {});
}

String EngineRenderScene::getTransferCmdBufferName() const
{
    return TCHAR("EngineRenderSceneTransferCmd_") + String::toString(frameCount % BUFFER_COUNT);
}

String EngineRenderScene::getCmdBufferName() const { return TCHAR("EngineRenderSceneCmd_") + String::toString(frameCount % BUFFER_COUNT); }

EngineRenderScene::EngineRenderScene(cbe::World *inWorld)
    : world(inWorld)
    , rtPool(BUFFER_COUNT)
{
    ComponentRenderSyncInfo syncInfo;
    for (cbe::Actor *actor : inWorld->getActors())
    {
        for (cbe::TransformLeafComponent *leafComp : actor->getLeafComponents())
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(leafComp))
            {
                syncInfo.compsAdded.emplace_back(renderComp);
            }
        }
    }
    ENQUEUE_RENDER_COMMAND(EngineRenderSceneCtor)
    (
        [syncInfo = std::move(syncInfo),
         this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            syncWorldCompsRenderThread(syncInfo, cmdList, graphicsInstance, graphicsHelper);
            initRenderThread(cmdList, graphicsInstance, graphicsHelper);
        }
    );

    // No need to clear the bindings as EngineRenderScene lifetime is less than that of World itself
    inWorld->onLeafCompAdded.bindLambda(
        [this](cbe::Object *compObj)
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(compObj))
            {
                componentUpdates.compsAdded.emplace_back(renderComp);
            }
        }
    );
    inWorld->onLeafCompAdded.bindLambda(
        [this](cbe::Object *compObj)
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(compObj))
            {
                componentUpdates.compsRemoved.emplace_back(renderComp->getObjectData().path);
                // Remove component from added components list if both happening in same frame
                std::erase(componentUpdates.compsAdded, compObj);
                std::erase(componentUpdates.recreateComps, compObj);
            }
        }
    );

    // TODO(JESLAS) : Enable below once invalidating components are added
    // inWorld->onLeafInvalidated.bindLambda(
    //    [this](cbe::Object *compObj)
    //    {
    //        if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(compObj))
    //        {
    //            componentUpdates.recreateComps.emplace_back(renderComp);
    //        }
    //    }
    //);

    inWorld->onLeafsTransformed.bindLambda(
        [this](ArrayView<cbe::TransformLeafComponent *> compObj)
        {
            for (cbe::TransformLeafComponent *leaf : compObj)
            {
                if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(leaf))
                {
                    componentUpdates.compsTransformed.emplace_back(renderComp);
                }
            }
        }
    );
}

EngineRenderScene::~EngineRenderScene() { clearScene(); }

void EngineRenderScene::initRenderThread(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    IRenderInterfaceModule *renderModule = IRenderInterfaceModule::get();
    RenderManager *renderMan = renderModule->getRenderManager();

    const PipelineBase *defaultShaderPipeline
        = renderMan->getGlobalRenderingContext()->getDefaultPipeline(TCHAR("Default"), EVertexType::StaticMesh, ERenderPassFormat::Multibuffer);
    if (defaultShaderPipeline == nullptr)
    {
        LOG_ERROR("EngineRenderScene", "Default shader pipeline not found!", frameCount);
        return;
    }
    const PipelineBase *debugDrawDepthPipeline = renderMan->getGlobalRenderingContext()->getDefaultPipeline(TCHAR("DebugVisDepthTexture"));
    const PipelineBase *drawTextureQuadPipeline = renderMan->getGlobalRenderingContext()->getDefaultPipeline(TCHAR("DrawQuadFromTexture"));
    alertOncef(debugDrawDepthPipeline && drawTextureQuadPipeline, "Necessary shaders to draw to final texture is not found");
    for (uint32 bufferIdx = 0; bufferIdx != BUFFER_COUNT; ++bufferIdx)
    {
        String idxStr = String::toString(bufferIdx);
        ShaderParametersRef bindlessParam = graphicsHelper->createShaderParameters(
            graphicsInstance, defaultShaderPipeline->getParamLayoutAtSet(ShaderParameterUtility::BINDLESS_SET)
        );
        bindlessParam->setResourceName(world.getObjectName() + TCHAR("_Bindless_") + idxStr);
        bindlessParam->init();
        bindlessSet.push(bindlessParam);

        ShaderParametersRef sceneViewParam = graphicsHelper->createShaderParameters(
            graphicsInstance, defaultShaderPipeline->getParamLayoutAtSet(ShaderParameterUtility::VIEW_UNIQ_SET)
        );
        sceneViewParam->setResourceName(world.getObjectName() + TCHAR("_View_") + idxStr);
        sceneViewParam->init();
        sceneViewParams.push(sceneViewParam);

        if (debugDrawDepthPipeline)
        {
            ShaderParametersRef depthDrawParam
                = graphicsHelper->createShaderParameters(graphicsInstance, debugDrawDepthPipeline->getParamLayoutAtSet(0));
            depthDrawParam->setResourceName(world.getObjectName() + TCHAR("_DepthDraw_") + idxStr);
            depthDrawParam->init();
            depthResolveParams.push(depthDrawParam);
        }
        if (drawTextureQuadPipeline)
        {
            ShaderParametersRef quadDrawParam
                = graphicsHelper->createShaderParameters(graphicsInstance, drawTextureQuadPipeline->getParamLayoutAtSet(0));
            quadDrawParam->setResourceName(world.getObjectName() + TCHAR("_QuadTextureDraw_") + idxStr);
            quadDrawParam->init();
            colorResolveParams.push(quadDrawParam);
        }
    }

    performTransferCopies(cmdList, graphicsInstance, graphicsHelper);
}

void EngineRenderScene::clearScene()
{
    RenderThreadEnqueuer::execInRenderThreadAndWait(
        [this](IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            rtPool.clearPool(cmdList);

            // Force cancel async updates
            bVertexUpdating = false;
            bMaterialsUpdating = false;
            bInstanceParamsUpdating = false;
        }
    );

    frameCount = 0;
    world.reset();
    compsRenderInfo.clear();
    componentToRenderInfo.clear();
    componentUpdates.clear();

    // Clear each render resources now that RTs are cleared and waited
    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        vertexBuffers[vertType] = {};
        instancesData[vertType] = {};
    }
    shaderToMaterials.clear();

    bindlessSet.reset();

    sceneViewParams = {};
    colorResolveParams = {};
    depthResolveParams = {};

    for (uint32 i = 0; i != ERendererIntermTexture::MaxCount; ++i)
    {
        frameTextures[i] = {};
    }

    // TODO(Jeslas) : Clear scene
}

void EngineRenderScene::renderTheScene(RenderSceneViewParams viewParams)
{
    // start the rendering in Renderer
    ENQUEUE_RENDER_COMMAND(RenderScene)
    (
        [viewParams, compUpdates = std::move(componentUpdates),
         this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            syncWorldCompsRenderThread(compUpdates, cmdList, graphicsInstance, graphicsHelper);
            updateVisibility(viewParams);

            createNextDrawList(viewParams, cmdList, graphicsInstance, graphicsHelper);

            cmdList->finishCmd(getCmdBufferName());
            frameTextures[ERendererIntermTexture::FinalColor] = getFinalColor(cmdList, viewParams.viewportSize);
            renderTheSceneRenderThread(viewParams, cmdList, graphicsInstance, graphicsHelper);
            performTransferCopies(cmdList, graphicsInstance, graphicsHelper);
            // Clear once every buffer cycle
            if ((frameCount % BUFFER_COUNT) == 0)
            {
                rtPool.clearUnused(cmdList);
            }
            frameCount++;
        }
    );
}

const IRenderTargetTexture *EngineRenderScene::getLastRTResolved() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return &frameTextures[ERendererIntermTexture::FinalColor];
}

void EngineRenderScene::onLastRTCopied()
{
    // TODO(Jeslas) : OnLastRTCopied, Is this needed?
}

FORCE_INLINE void EngineRenderScene::addMeshRef(EVertexType::Type vertType, cbe::ObjectPath meshPath, SizeT compRenderInfoIdx)
{
    VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];

    auto meshItr = sceneVerts.meshes.find(meshPath);
    if (meshItr != sceneVerts.meshes.end())
    {
        meshItr->second.refs++;
    }
    else if (bVertexUpdating)
    {
        sceneVerts.meshesToAdd.emplace_back(meshPath, compRenderInfoIdx);
    }
    else
    {
        debugAssert(
            compRenderInfo.cpuIdxBuffer.isValid() && compRenderInfo.cpuVertBuffer.isValid() && compRenderInfo.cpuIdxBuffer->isValid()
            && compRenderInfo.cpuVertBuffer->isValid() && compRenderInfo.cpuVertBuffer->bufferStride() > 1
            && compRenderInfo.cpuIdxBuffer->bufferStride() > 1
        );
        cbe::Object *mesh = cbe::get(meshPath.getFullPath().getChar());
        if (!cbe::isValid(mesh))
        {
            debugAssert(!sceneVerts.meshes.contains(meshPath));
            return;
        }

        SizeT idxOffset, vertOffset;
        bool bIdxAlloced = sceneVerts.idxsAllocTracker.allocate(compRenderInfo.cpuIdxBuffer->bufferCount(), 1, idxOffset);
        bool bVertAlloced = sceneVerts.vertsAllocTracker.allocate(compRenderInfo.cpuVertBuffer->bufferCount(), 1, vertOffset);
        if (bIdxAlloced && bVertAlloced)
        {
            MeshVertexView &vertView = sceneVerts.meshes[meshPath];
            vertView.idxOffset = idxOffset;
            vertView.idxCount = compRenderInfo.cpuIdxBuffer->bufferCount();
            vertView.vertOffset = vertOffset;
            vertView.vertCount = compRenderInfo.cpuVertBuffer->bufferCount();
            vertView.refs = 1;

            uint32 vertStride = compRenderInfo.cpuVertBuffer->bufferStride(), idxStride = compRenderInfo.cpuIdxBuffer->bufferStride();

            sceneVerts.copies.emplace_back(BatchCopyBufferInfo{
                .src = compRenderInfo.cpuVertBuffer,
                .dst = sceneVerts.vertices,
                .copyInfo = {0, vertView.vertOffset * vertStride, uint32(vertView.vertCount * vertStride)}
            });

            sceneVerts.copies.emplace_back(BatchCopyBufferInfo{
                .src = compRenderInfo.cpuIdxBuffer,
                .dst = sceneVerts.indices,
                .copyInfo = {0, vertView.idxOffset * idxStride, uint32(vertView.idxCount * idxStride)}
            });
        }
        else
        {
            // Not enough space allocate new buffers and copy
            sceneVerts.meshesToAdd.emplace_back(meshPath, compRenderInfoIdx);
        }
    }
}

FORCE_INLINE void EngineRenderScene::removeMeshRef(EVertexType::Type vertType, MeshVertexView &meshVertView, cbe::ObjectPath meshID)
{
    VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
    debugAssert(meshVertView.refs >= 1);

    meshVertView.refs--;
    if (meshVertView.refs == 0)
    {
        if (bVertexUpdating)
        {
            sceneVerts.meshesToRemove.emplace_back(meshID);
        }
        else
        {
            sceneVerts.vertsAllocTracker.deallocate(meshVertView.vertOffset, meshVertView.vertCount);
            sceneVerts.idxsAllocTracker.deallocate(meshVertView.idxOffset, meshVertView.idxCount);
            sceneVerts.meshes.erase(meshID);
        }
    }
}

FORCE_INLINE void EngineRenderScene::createInstanceCopies(
    InstanceParamsPerVertType &vertInstanceData, const ComponentRenderInfo &compRenderInfo, IRenderCommandList *cmdList,
    IGraphicsInstance *graphicsInstance
) const
{
    // TODO(Jeslas) : This is test code, must be changing for each vertex type
    debugAssert(vertInstanceData.instanceData.isValid());
    StringID paramPath[] = { INSTANCES_BUFFER_NAME, STRID("instances"), STRID("model") };
    uint32 indices[] = { 0, uint32(instanceIdxToVectorIdx(compRenderInfo.tfIndex)), 0 };
    vertInstanceData.shaderParameter->setMatrixAtPath(paramPath, indices, compRenderInfo.worldTf.getTransformMatrix());
    paramPath[2] = STRID("invModel");
    vertInstanceData.shaderParameter->setMatrixAtPath(
        paramPath, indices, compRenderInfo.worldTf.inverseNonUniformScaled().getTransformMatrix()
    );
    paramPath[2] = STRID("shaderUniqIdx");
    vertInstanceData.shaderParameter->setIntAtPath(paramPath, indices, uint32(materialIdxToVectorIdx(compRenderInfo.materialIndex)));

    vertInstanceData.shaderParameter->pullBufferParamUpdates(vertInstanceData.hostToBufferCopies, cmdList, graphicsInstance);
}

FORCE_INLINE void EngineRenderScene::addCompInstanceData(SizeT compRenderInfoIdx)
{
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
    debugAssert(compRenderInfo.tfIndex == 0);

    InstanceParamsPerVertType &vertInstanceData = instancesData[compRenderInfo.vertexType];
    if (bInstanceParamsUpdating)
    {
        vertInstanceData.compIdxToAdd.emplace_back(compRenderInfoIdx);
    }
    else
    {
        SizeT instanceIdx;
        if (vertInstanceData.allocTracker.allocate(1, 1, instanceIdx))
        {
            compRenderInfo.tfIndex = vectorIdxToInstanceIdx(instanceIdx);
            // Material Index must be valid when creating instance
            debugAssert(compRenderInfo.materialIndex != 0);

            IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get();
            createInstanceCopies(
                vertInstanceData, compRenderInfo, renderInterface->getRenderManager()->getRenderCmds(),
                renderInterface->currentGraphicsInstance()
            );
        }
        else
        {
            // Not enough space allocate new buffers and copy
            vertInstanceData.compIdxToAdd.emplace_back(compRenderInfoIdx);
        }
    }
}

FORCE_INLINE void EngineRenderScene::removeInstanceDataAt(EVertexType::Type vertexType, SizeT instanceIdx)
{
    SizeT instanceVectorIdx = instanceIdxToVectorIdx(instanceIdx);

    InstanceParamsPerVertType &instanceParams = instancesData[vertexType];
    debugAssert(instanceParams.allocTracker.isRangeAllocated(instanceVectorIdx, 1));

    if (bInstanceParamsUpdating)
    {
        instanceParams.instanceIdxToRemove.emplace_back(instanceIdx);
    }
    else
    {
        instanceParams.allocTracker.deallocate(instanceVectorIdx, 1);
    }
}

FORCE_INLINE void EngineRenderScene::createMaterialCopies(
    MaterialShaderParams &shaderMats, SizeT materialIdx, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance
) const
{
    // TODO(Jeslas) : This is test code, must be unique per shader
    debugAssert(shaderMats.shaderParameter.isValid() && shaderMats.materialData.isValid());
    StringID paramPath[] = { MATERIAL_BUFFER_NAME, STRID("meshData"), STRID("meshColor") };
    uint32 indices[] = { 0, uint32(materialIdxToVectorIdx(materialIdx)), 0 };
    shaderMats.shaderParameter->setVector4AtPath(paramPath, indices, LinearColorConst::random());
    paramPath[2] = STRID("roughness");
    shaderMats.shaderParameter->setFloatAtPath(paramPath, indices, Math::random());
    paramPath[2] = STRID("metallic");
    shaderMats.shaderParameter->setFloatAtPath(paramPath, indices, Math::random());

    shaderMats.shaderParameter->pullBufferParamUpdates(shaderMats.hostToMatCopies, cmdList, graphicsInstance);
}

FORCE_INLINE void EngineRenderScene::addCompMaterialData(SizeT compRenderInfoIdx)
{
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
    debugAssert(compRenderInfo.materialIndex == 0);

    if (!shaderToMaterials.contains(compRenderInfo.shaderName))
    {
        shaderToMaterials[compRenderInfo.shaderName].compIdxToAdd.emplace_back(compRenderInfoIdx);
        return;
    }

    MaterialShaderParams &shaderMats = shaderToMaterials[compRenderInfo.shaderName];
    auto shaderMatsIdxItr = shaderMats.materialToIdx.find(compRenderInfo.matObjPath);
    if (shaderMatsIdxItr != shaderMats.materialToIdx.end())
    {
        compRenderInfo.materialIndex = vectorIdxToMaterialIdx(shaderMatsIdxItr->second);
        shaderMats.materialRefs[shaderMatsIdxItr->second]++;
        return;
    }
    else if (bMaterialsUpdating)
    {
        shaderMats.compIdxToAdd.emplace_back(compRenderInfoIdx);
    }
    else
    {
        // TODO(Jeslas) : Uncomment below once proper material asset is added
        // cbe::Object *materialInst = cbe::get(compRenderInfo.materialID);
        // if (!cbe::isValid(materialInst))
        //{
        //     debugAssert(!shaderMats.materialToIdx.contains(compRenderInfo.materialID));
        //     return;
        // }

        SizeT matIdx;
        if (shaderMats.materialAllocTracker.allocate(1, 1, matIdx))
        {
            shaderMats.materialToIdx[compRenderInfo.matObjPath] = matIdx;
            shaderMats.materialRefs[matIdx] = 1;

            compRenderInfo.materialIndex = vectorIdxToMaterialIdx(matIdx);
            IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get();
            createMaterialCopies(
                shaderMats, compRenderInfo.materialIndex, renderInterface->getRenderManager()->getRenderCmds(),
                renderInterface->currentGraphicsInstance()
            );
        }
        else
        {
            // Not enough space allocate new buffers and copy
            shaderMats.compIdxToAdd.emplace_back(compRenderInfoIdx);
        }
    }
}

FORCE_INLINE void EngineRenderScene::removeMaterialAt(SizeT matVectorIdx, cbe::ObjectPath materialID, MaterialShaderParams &shaderMats)
{
    debugAssert(shaderMats.materialAllocTracker.isRangeAllocated(matVectorIdx, 1) && shaderMats.materialRefs[matVectorIdx] >= 1);
    shaderMats.materialRefs[matVectorIdx]--;
    if (shaderMats.materialRefs[matVectorIdx] == 0)
    {
        if (bMaterialsUpdating)
        {
            shaderMats.materialIDToRemove.emplace_back(materialID);
        }
        else
        {
            shaderMats.materialAllocTracker.deallocate(matVectorIdx, 1);
            shaderMats.materialToIdx.erase(materialID);
        }
    }
}

void EngineRenderScene::addRenderComponents(const std::vector<cbe::RenderableComponent *> &renderComps)
{
    for (cbe::RenderableComponent *compToAdd : renderComps)
    {
        if (!cbe::isValid(compToAdd))
        {
            continue;
        }
        cbe::ObjectPath compPath = cbe::ObjectPath(compToAdd);
        auto compToIdxItr = componentToRenderInfo.find(compPath);
        if (compToIdxItr == componentToRenderInfo.end())
        {
            SizeT idx = compsRenderInfo.get();
            createRenderInfo(compToAdd, idx);
            if (compsRenderInfo[idx].meshObjPath.isValid())
            {
                debugAssert(compsRenderInfo[idx].cpuVertBuffer.isValid() && compsRenderInfo[idx].cpuIdxBuffer.isValid());
                addMeshRef(compsRenderInfo[idx].vertexType, compsRenderInfo[idx].meshObjPath, idx);
            }
            componentToRenderInfo[compPath] = idx;
        }
    }
}

void EngineRenderScene::removeRenderComponents(const std::vector<String> &renderComps)
{
    for (const String &compToRemove : renderComps)
    {
        auto compToIdxItr = componentToRenderInfo.find(cbe::ObjectPath{ compToRemove.getChar() });
        if (compToIdxItr != componentToRenderInfo.end())
        {
            SizeT idx = compToIdxItr->second;
            cbe::ObjectPath currMesh = compsRenderInfo[idx].meshObjPath;
            EVertexType::Type currVertType = compsRenderInfo[idx].vertexType;

            if (currMesh.isValid())
            {
                removeMeshRef(currVertType, vertexBuffers[currVertType].meshes[currMesh], currMesh);
            }
            cbe::RenderableComponent *comp = cbe::cast<cbe::RenderableComponent>(cbe::get(compToRemove.getChar()));
            destroyRenderInfo(comp, idx);
            componentToRenderInfo.erase(compToIdxItr);
            compsRenderInfo.reset(idx);
        }
    }
}

void EngineRenderScene::recreateRenderComponents(const std::vector<cbe::RenderableComponent *> &renderComps)
{
    for (cbe::RenderableComponent *compToRecreate : renderComps)
    {
        if (!cbe::isValid(compToRecreate))
        {
            continue;
        }
        cbe::ObjectPath compToRecreatePath{ compToRecreate };
        auto compToIdxItr = componentToRenderInfo.find(compToRecreatePath);
        if (compToIdxItr == componentToRenderInfo.end())
        {
            SizeT idx = compsRenderInfo.get();
            createRenderInfo(compToRecreate, idx);

            if (compsRenderInfo[idx].meshObjPath.isValid())
            {
                debugAssert(compsRenderInfo[idx].cpuVertBuffer.isValid() && compsRenderInfo[idx].cpuIdxBuffer.isValid());
                addMeshRef(compsRenderInfo[idx].vertexType, compsRenderInfo[idx].meshObjPath, idx);
            }
            componentToRenderInfo[compToRecreatePath] = idx;
        }
        else
        {
            SizeT idx = compToIdxItr->second;
            const cbe::ObjectPath &currMesh = compsRenderInfo[idx].meshObjPath;
            EVertexType::Type currVertType = compsRenderInfo[idx].vertexType;

            destroyRenderInfo(compToRecreate, idx);
            createRenderInfo(compToRecreate, idx);

            const cbe::ObjectPath &newMesh = compsRenderInfo[idx].meshObjPath;
            if (currMesh != newMesh)
            {
                debugAssert(currVertType == compsRenderInfo[idx].vertexType);
                if (currMesh.isValid())
                {
                    removeMeshRef(currVertType, vertexBuffers[currVertType].meshes[currMesh], currMesh);
                }
                if (newMesh.isValid())
                {
                    addMeshRef(currVertType, newMesh, idx);
                }
            }
        }
    }
}

void EngineRenderScene::updateTfComponents(
    const std::vector<cbe::RenderableComponent *> &comps, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance
)
{
    for (cbe::RenderableComponent *renderComp : comps)
    {
        if (!cbe::isValidFast(renderComp))
        {
            continue;
        }

        cbe::ObjectPath updateTfPath{ renderComp };
        auto compRenderIdxItr = componentToRenderInfo.find(updateTfPath);
        if (compRenderIdxItr != componentToRenderInfo.cend())
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderIdxItr->second];
            // TODO(Jeslas) : Getting world tf here is safe?
            compRenderInfo.worldTf = renderComp->getWorldTransform();
            compRenderInfo.worldBound = AABB();
            Vector3 aabbCorners[8];
            renderComp->getLocalBound().boundCorners(aabbCorners);
            for (uint32 i = 0; i != ARRAY_LENGTH(aabbCorners); ++i)
            {
                compRenderInfo.worldBound.grow(compRenderInfo.worldTf.transformPoint(aabbCorners[i]));
            }

            if (compRenderInfo.tfIndex != 0)
            {
                createInstanceCopies(instancesData[compRenderInfo.vertexType], compRenderInfo, cmdList, graphicsInstance);
            }
        }
    }
}

void EngineRenderScene::createRenderInfo(cbe::RenderableComponent *comp, SizeT compRenderInfoIdx)
{
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
    compRenderInfo.compObjPath = comp;
    comp->setupRenderInfo(compRenderInfo);

    // TODO(Jeslas): Remove below demo code
    compRenderInfo.shaderName = TCHAR("SingleColor");
    compRenderInfo.matObjPath = comp->getActor();
    addCompMaterialData(compRenderInfoIdx);
    addCompInstanceData(compRenderInfoIdx);
}

void EngineRenderScene::destroyRenderInfo(const cbe::RenderableComponent *comp, SizeT compRenderInfoIdx)
{
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];

    if (compRenderInfo.materialIndex != 0)
    {
        removeMaterialAt(
            materialIdxToVectorIdx(compRenderInfo.materialIndex), compRenderInfo.matObjPath, shaderToMaterials[compRenderInfo.shaderName]
        );
        compRenderInfo.materialIndex = 0;
    }
    if (compRenderInfo.tfIndex != 0)
    {
        removeInstanceDataAt(compRenderInfo.vertexType, compRenderInfo.tfIndex);
        compRenderInfo.tfIndex = 0;
    }

    if (cbe::isValid(comp))
    {
        comp->clearRenderInfo(compRenderInfo);
    }
}

void EngineRenderScene::performTransferCopies(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *)
{
    // Perform other transfers
    bool bNeedTransferCommand = !bVertexUpdating || !bMaterialsUpdating || !bInstanceParamsUpdating;

    if (!bNeedTransferCommand)
    {
        return;
    }

    String cmdBufferName = getTransferCmdBufferName();
    cmdList->finishCmd(cmdBufferName);
    const GraphicsResource *cmdBuffer = cmdList->startCmd(cmdBufferName, EQueueFunction::Transfer, true);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, RenderSceneTransfer);
        std::vector<BatchCopyBufferData> perFrameCopies;

        // Copying vertex and index copies
        if (!bVertexUpdating)
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, CopySceneVertexInputs);
            std::vector<BatchCopyBufferInfo> allCopies;
            for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
            {
                allCopies.insert(allCopies.end(), vertexBuffers[vertType].copies.cbegin(), vertexBuffers[vertType].copies.cend());
                vertexBuffers[vertType].copies.clear();
            }
            if (!allCopies.empty())
            {
                vertIdxBufferCopied.set();
                cmdList->cmdCopyBuffer(cmdBuffer, allCopies);
            }
        }

        // Copying materials and draw list updates
        if (!bMaterialsUpdating)
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, CopySceneMaterials);
            std::vector<BatchCopyBufferInfo> allCopies;
            std::vector<BatchCopyBufferData> allHostToDeviceCopies;
            for (std::pair<const String, MaterialShaderParams> &shaderMaterials : shaderToMaterials)
            {
                allCopies.insert(allCopies.end(), shaderMaterials.second.materialCopies.cbegin(), shaderMaterials.second.materialCopies.cend());
                allHostToDeviceCopies.insert(
                    allHostToDeviceCopies.end(), shaderMaterials.second.hostToMatCopies.cbegin(), shaderMaterials.second.hostToMatCopies.cend()
                );
                shaderMaterials.second.materialCopies.clear();
                shaderMaterials.second.hostToMatCopies.clear();
                shaderMaterials.second.bMatsCopied = true;
            }
            if (!allCopies.empty())
            {
                cmdList->cmdCopyBuffer(cmdBuffer, allCopies);
            }
            if (!allHostToDeviceCopies.empty())
            {
                cmdList->cmdCopyToBuffer(cmdBuffer, allHostToDeviceCopies);
            }
        }

        // Copying instance data
        if (!bInstanceParamsUpdating)
        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, CopyPerVertInstanceData);
            std::vector<BatchCopyBufferInfo> allCopies;
            std::vector<BatchCopyBufferData> allHostToDeviceCopies;
            for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
            {
                allCopies.insert(allCopies.end(), instancesData[vertType].copies.cbegin(), instancesData[vertType].copies.cend());
                allHostToDeviceCopies.insert(
                    allHostToDeviceCopies.end(), instancesData[vertType].hostToBufferCopies.cbegin(),
                    instancesData[vertType].hostToBufferCopies.cend()
                );
                instancesData[vertType].copies.clear();
                instancesData[vertType].hostToBufferCopies.clear();
            }
            if (!allCopies.empty())
            {
                cmdList->cmdCopyBuffer(cmdBuffer, allCopies);
                instanceDataCopied.set();
            }
            if (!allHostToDeviceCopies.empty())
            {
                cmdList->cmdCopyToBuffer(cmdBuffer, allHostToDeviceCopies);
                instanceDataCopied.set();
            }
        }

        {
            SCOPED_CMD_MARKER(cmdList, cmdBuffer, PerFrameCopies);

            for (std::pair<const String, MaterialShaderParams> &shaderMaterials : shaderToMaterials)
            {
                perFrameCopies.insert(
                    perFrameCopies.end(), shaderMaterials.second.drawListCopies.cbegin(), shaderMaterials.second.drawListCopies.cend()
                );
                shaderMaterials.second.drawListCopies.clear();
                shaderMaterials.second.drawListCopied.set();
            }
            // Always next one will be written to
            bindlessSet.peek(1)->pullBufferParamUpdates(perFrameCopies, cmdList, graphicsInstance);
            sceneViewParams.peek(1)->pullBufferParamUpdates(perFrameCopies, cmdList, graphicsInstance);

            if (!perFrameCopies.empty())
            {
                cmdList->cmdCopyToBuffer(cmdBuffer, perFrameCopies);
            }
        }
    }
    cmdList->cmdReleaseQueueResources(cmdBuffer, EQueueFunction::Graphics);
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo2 submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, submitInfo);
}

void EngineRenderScene::updateVisibility(const RenderSceneViewParams &viewParams)
{
    CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("UpdateVisibility"));

    const uint32 totalCompCapacity = uint32(compsRenderInfo.totalCount());
    compsVisibility.resize(totalCompCapacity);

    compsVisibility.resetRange(0, totalCompCapacity);

    ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();

    // Below ds must be either cache line separated boolean or atomic bool to avoid memory stomping
    // Will be inside frustum only if every other condition to render a mesh is valid
    std::vector<std::atomic_flag, CBEStlStackAllocatorExclusive<std::atomic_flag>> compsInsideFrustum(
        totalCompCapacity, appInstance->getRenderFrameAllocator()
    );

    Vector3 frustumCorners[8];
    Plane frustumPlanes[6];
    viewParams.view.frustumCorners(frustumCorners);
    viewParams.view.frustumPlanes(frustumPlanes);

    copat::parallelFor(
        appInstance->jobSystem,
        copat::DispatchFunctionType::createLambda(
            [this, &compsInsideFrustum, &frustumPlanes, &frustumCorners](uint32 idx)
            {
                CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("CompVisibility"));
                if (!compsRenderInfo.isValid(idx))
                {
                    return;
                }

                const ComponentRenderInfo &compRenderInfo = compsRenderInfo[idx];
                if (vertexBuffers[compRenderInfo.vertexType].meshes.contains(compRenderInfo.meshObjPath) && compRenderInfo.tfIndex != 0
                    && compRenderInfo.materialIndex != 0)
                {
#if DISABLE_PER_FRAME_UPDATE
                    compsInsideFrustum[idx].test_and_set(std::memory_order::relaxed);
                    return;
#else
                    if (!compRenderInfo.worldBound.isValidAABB())
                    {
                        return;
                    }

                    Vector3 aabbCorners[8];
                    compRenderInfo.worldBound.boundCorners(aabbCorners);

                    // Ensure that box is not completely outside all frustum planes,
                    // Even if all point is outside 1 plane it will be completely out like AABB
                    for (const Plane &plane : ArrayView<Plane>{ frustumPlanes })
                    {
                        uint32 outsideFrustumCount = 0;
                        for (const Vector3 &corner : ArrayView<Vector3>{ aabbCorners })
                        {
                            outsideFrustumCount += (plane | corner) < 0 ? 1 : 0;
                        }
                        if (outsideFrustumCount == 8)
                        {
                            return;
                        }
                    }
                    // Now make sure that AABB is not passing through the frustum,
                    // Pass thru happens when any corner pts axial component is inside it corresponding AABB extremes
                    uint32 outsideExtremeCount[6] = { 0 };
                    for (const Vector3 &corner : ArrayView<Vector3>{ frustumCorners })
                    {
                        outsideExtremeCount[0] += corner.x() > compRenderInfo.worldBound.maxBound.x() ? 1 : 0;
                        outsideExtremeCount[1] += corner.x() < compRenderInfo.worldBound.minBound.x() ? 1 : 0;

                        outsideExtremeCount[2] += corner.y() > compRenderInfo.worldBound.maxBound.y() ? 1 : 0;
                        outsideExtremeCount[3] += corner.y() < compRenderInfo.worldBound.minBound.y() ? 1 : 0;

                        outsideExtremeCount[4] += corner.z() > compRenderInfo.worldBound.maxBound.z() ? 1 : 0;
                        outsideExtremeCount[5] += corner.z() < compRenderInfo.worldBound.minBound.z() ? 1 : 0;
                    }

                    if (outsideExtremeCount[0] != 8 && outsideExtremeCount[1] != 8 && outsideExtremeCount[2] != 8 && outsideExtremeCount[3] != 8
                        && outsideExtremeCount[4] != 8 && outsideExtremeCount[5] != 8)
                    {
                        compsInsideFrustum[idx].test_and_set(std::memory_order_relaxed);
                    }
#endif
                }
            }
        ),
        totalCompCapacity
    );

    CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("WriteVisibilityBits"));
    for (SizeT i = 0; i != totalCompCapacity;)
    {
        bool bIsSet = compsInsideFrustum[i].test(std::memory_order::relaxed);
        SizeT endIdx = i + 1;
        for (; endIdx != totalCompCapacity; ++endIdx)
        {
            if (bIsSet != compsInsideFrustum[endIdx].test(std::memory_order::relaxed))
            {
                break;
            }
        }
        if (bIsSet)
        {
            compsVisibility.setRange(i, endIdx - i);
        }
        else
        {
            compsVisibility.resetRange(i, endIdx - i);
        }

        i = endIdx;
    }
}

void EngineRenderScene::syncWorldCompsRenderThread(
    const ComponentRenderSyncInfo &compsUpdate, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
    const GraphicsHelperAPI *graphicsHelper
)
{
    ASSERT_INSIDE_RENDERTHREAD();

    removeRenderComponents(compsUpdate.compsRemoved);
    addRenderComponents(compsUpdate.compsAdded);
    recreateRenderComponents(compsUpdate.recreateComps);
    updateTfComponents(compsUpdate.compsTransformed, cmdList, graphicsInstance);

    if (!bVertexUpdating)
    {
        bool bRecreateSceneVerts = false;
        for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
        {
            VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
            if (!sceneVerts.meshesToRemove.empty())
            {
                auto meshesToRemove = std::move(sceneVerts.meshesToRemove);
                for (cbe::ObjectPath meshToRemove : meshesToRemove)
                {
                    auto meshViewItr = sceneVerts.meshes.find(meshToRemove);
                    if (meshViewItr != sceneVerts.meshes.cend())
                    {
                        removeMeshRef(EVertexType::Type(vertType), meshViewItr->second, meshToRemove);
                    }
                }
            }

            if (!sceneVerts.meshesToAdd.empty())
            {
                // Try adding the meshes directly to the current buffers, Any thing not copied will be added back to meshesToAdd list
                auto meshesToAdd = std::move(sceneVerts.meshesToAdd);
                for (const std::pair<const cbe::ObjectPath, SizeT> &meshToAdd : meshesToAdd)
                {
                    addMeshRef(EVertexType::Type(vertType), meshToAdd.first, meshToAdd.second);
                }

                bRecreateSceneVerts = bRecreateSceneVerts || !sceneVerts.meshesToAdd.empty();
            }
        }

        if (bRecreateSceneVerts)
        {
            recreateSceneVertexBuffers(cmdList, graphicsInstance, graphicsHelper);
        }
    }

    if (!bMaterialsUpdating)
    {
        bool bRecreateMaterials = false;
        for (std::pair<const String, MaterialShaderParams> &shaderMats : shaderToMaterials)
        {
            if (!shaderMats.second.materialIDToRemove.empty())
            {
                auto matIDsToRemove = std::move(shaderMats.second.materialIDToRemove);
                for (cbe::ObjectPath matIDToRemove : matIDsToRemove)
                {
                    debugAssert(shaderMats.second.materialToIdx.contains(matIDToRemove));
                    removeMaterialAt(shaderMats.second.materialToIdx[matIDToRemove], matIDToRemove, shaderMats.second);
                }
            }

            if (!shaderMats.second.compIdxToAdd.empty())
            {
                auto compsToAdd = std::move(shaderMats.second.compIdxToAdd);
                for (SizeT compIdxToAdd : compsToAdd)
                {
                    addCompMaterialData(compIdxToAdd);
                }

                bRecreateMaterials = bRecreateMaterials || !shaderMats.second.compIdxToAdd.empty();
            }
        }

        if (bRecreateMaterials)
        {
            recreateMaterialBuffers(cmdList, graphicsInstance, graphicsHelper);
        }
    }

    // Since Instance data depends on material index
    if (!bMaterialsUpdating && !bInstanceParamsUpdating)
    {
        bool bRecreateInstanceData = false;
        for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
        {
            InstanceParamsPerVertType &instanceParams = instancesData[vertType];
            if (!instanceParams.instanceIdxToRemove.empty())
            {
                auto instancesToRemove = std::move(instanceParams.instanceIdxToRemove);
                for (SizeT instToRemove : instancesToRemove)
                {
                    removeInstanceDataAt(EVertexType::Type(vertType), instToRemove);
                }
            }

            if (!instanceParams.compIdxToAdd.empty())
            {
                auto compsToAdd = std::move(instanceParams.compIdxToAdd);
                for (SizeT compIdxToAdd : compsToAdd)
                {
                    addCompInstanceData(compIdxToAdd);
                }

                bRecreateInstanceData = bRecreateInstanceData || !instanceParams.compIdxToAdd.empty();
            }
        }

        if (bRecreateInstanceData)
        {
            recreateInstanceBuffers(cmdList, graphicsInstance, graphicsHelper);
        }
    }
}

copat::JobSystemFuncAwaiter EngineRenderScene::recreateSceneVertexBuffers(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    bVertexUpdating = true;

    VerticesPerVertType newBuffers[VERTEX_TYPE_COUNT];
    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
        VerticesPerVertType &newSceneVerts = newBuffers[vertType];
        debugAssert(sceneVerts.meshesToRemove.empty());

        if (sceneVerts.meshesToAdd.empty())
        {
            continue;
        }

        newSceneVerts.meshesToAdd = std::move(sceneVerts.meshesToAdd);
        debugAssertf(compsRenderInfo.isValid(newSceneVerts.meshesToAdd[0].second), "Component render info must be valid when adding new mesh!");

        // Find total additional vertices and indices needs to be added
        uint32 addVertsCount = 0, addIdxsCount = 0, vertexStride, idxStride;
        const ComponentRenderInfo &tempRenderInfo = compsRenderInfo[newSceneVerts.meshesToAdd[0].second];
        debugAssert(
            tempRenderInfo.cpuIdxBuffer.isValid() && tempRenderInfo.cpuVertBuffer.isValid() && tempRenderInfo.cpuIdxBuffer->isValid()
            && tempRenderInfo.cpuVertBuffer->isValid() && tempRenderInfo.cpuVertBuffer->bufferStride() > 1
            && tempRenderInfo.cpuIdxBuffer->bufferStride() > 1
        );
        vertexStride = tempRenderInfo.cpuVertBuffer->bufferStride();
        idxStride = tempRenderInfo.cpuIdxBuffer->bufferStride();

        std::unordered_set<cbe::ObjectPath> meshesAdded;
        meshesAdded.reserve(newSceneVerts.meshesToAdd.size());
        for (uint64 addIdx = 0; addIdx != newSceneVerts.meshesToAdd.size();)
        {
            const std::pair<const cbe::ObjectPath, SizeT> &meshToAdd = newSceneVerts.meshesToAdd[addIdx];
            if (meshesAdded.insert(meshToAdd.first).second)
            {
                const ComponentRenderInfo &compRenderInfo = compsRenderInfo[newSceneVerts.meshesToAdd[addIdx].second];
                addVertsCount += compRenderInfo.cpuVertBuffer->bufferCount();
                addIdxsCount += compRenderInfo.cpuIdxBuffer->bufferCount();
                ++addIdx;
            }
            else
            {
                std::iter_swap(newSceneVerts.meshesToAdd.begin() + addIdx, newSceneVerts.meshesToAdd.end() - 1);
                newSceneVerts.meshesToAdd.pop_back();
            }
        }

        // Setup newSceneVerts data for new size and counts
        uint64 newVertsCount = sceneVerts.vertsAllocTracker.size() + addVertsCount,
               newIdxsCount = sceneVerts.idxsAllocTracker.size() + addIdxsCount;
        newVertsCount = Math::toHigherPowOf2(newVertsCount);
        newIdxsCount = Math::toHigherPowOf2(newIdxsCount);

        newSceneVerts.vertsAllocTracker.resize(newVertsCount);
        newSceneVerts.idxsAllocTracker.resize(newIdxsCount);

        newSceneVerts.meshes.reserve(sceneVerts.meshes.size() + newSceneVerts.meshesToAdd.size());

        newSceneVerts.vertices = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, vertexStride, uint32(newVertsCount));
        newSceneVerts.vertices->setResourceName(
            world.getObjectName() + TCHAR("_") + EVertexType::toString(EVertexType::Type(vertType)) + TCHAR("_Vertices"));
        newSceneVerts.vertices->init();

        newSceneVerts.indices = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, idxStride, uint32(newIdxsCount));
        newSceneVerts.indices->setResourceName(
            world.getObjectName() + TCHAR("_") + EVertexType::toString(EVertexType::Type(vertType)) + TCHAR("_Indices"));
        newSceneVerts.indices->init();
    }

    // Now switch to some worker thread to finish all copies, current scene vertices will not be modified until bVertexUpdating is set false
    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::WorkerThreads>{};

    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
        VerticesPerVertType &newSceneVerts = newBuffers[vertType];
        if (newSceneVerts.meshesToAdd.empty())
        {
            continue;
        }
        uint32 vertexStride = newSceneVerts.vertices->bufferStride(), idxStride = newSceneVerts.indices->bufferStride();

        // It is okay to directly modify the copies as it will not be modified until bVertexUpdating is false
        sceneVerts.copies.reserve(sceneVerts.copies.size() + sceneVerts.meshes.size() + newSceneVerts.meshesToAdd.size());
        uint64 vertOffset = 0, idxOffset = 0;
        for (const std::pair<const cbe::ObjectPath, MeshVertexView> &meshViewPair : sceneVerts.meshes)
        {
            MeshVertexView &newVertexView = newSceneVerts.meshes[meshViewPair.first];
            newVertexView.idxCount = meshViewPair.second.idxCount;
            newVertexView.vertCount = meshViewPair.second.vertCount;
            newVertexView.idxOffset = idxOffset;
            newVertexView.vertOffset = vertOffset;

            BatchCopyBufferInfo copyInfo;
            copyInfo.dst = newSceneVerts.vertices;
            copyInfo.src = sceneVerts.vertices;
            copyInfo.copyInfo = { meshViewPair.second.vertOffset * vertexStride, newVertexView.vertOffset * vertexStride,
                                  uint32(newVertexView.vertCount * vertexStride) };
            sceneVerts.copies.emplace_back(std::move(copyInfo));

            copyInfo.dst = newSceneVerts.indices;
            copyInfo.src = sceneVerts.indices;
            copyInfo.copyInfo = { meshViewPair.second.idxOffset * idxStride, newVertexView.idxOffset * idxStride,
                                  uint32(newVertexView.idxCount * idxStride) };
            sceneVerts.copies.emplace_back(std::move(copyInfo));

            vertOffset += newVertexView.vertCount;
            idxOffset += newVertexView.idxCount;
        }

        for (const std::pair<const cbe::ObjectPath, SizeT> &meshToAdd : newBuffers[vertType].meshesToAdd)
        {
            MeshVertexView &newVertexView = newSceneVerts.meshes[meshToAdd.first];
            const ComponentRenderInfo &compRenderInfo = compsRenderInfo[meshToAdd.second];
            newVertexView.idxCount = compRenderInfo.cpuIdxBuffer->bufferCount();
            newVertexView.vertCount = compRenderInfo.cpuVertBuffer->bufferCount();
            newVertexView.idxOffset = idxOffset;
            newVertexView.vertOffset = vertOffset;
            newVertexView.refs = 1;

            BatchCopyBufferInfo copyInfo;
            copyInfo.dst = newSceneVerts.vertices;
            copyInfo.src = compRenderInfo.cpuVertBuffer;
            copyInfo.copyInfo = { 0, newVertexView.vertOffset * vertexStride, uint32(newVertexView.vertCount * vertexStride) };
            sceneVerts.copies.emplace_back(std::move(copyInfo));

            copyInfo.dst = newSceneVerts.indices;
            copyInfo.src = compRenderInfo.cpuIdxBuffer;
            copyInfo.copyInfo = { 0, newVertexView.idxOffset * idxStride, uint32(newVertexView.idxCount * idxStride) };
            sceneVerts.copies.emplace_back(std::move(copyInfo));

            vertOffset += newVertexView.vertCount;
            idxOffset += newVertexView.idxCount;
        }

        // Now mark the entire allocated region index and vertex allocations tracker
        uint64 vertAllocedOffset, idxAllocedOffset;
        bool bVertAlloced = newSceneVerts.vertsAllocTracker.allocate(vertOffset, 1, vertAllocedOffset);
        bool bIdxAlloced = newSceneVerts.idxsAllocTracker.allocate(idxOffset, 1, idxAllocedOffset);
        debugAssert(bVertAlloced && bIdxAlloced);
    }

    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::RenderThread>{};

    // If vertex updating is reset in render thread it means render thread has forcefully rejected any new updates
    if (!bVertexUpdating)
    {
        LOG_DEBUG("EngineRenderScene", "Forced aborting scene vertex update merge!");
        co_return;
    }

    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
        VerticesPerVertType &newSceneVerts = newBuffers[vertType];

        if (newSceneVerts.meshesToAdd.empty())
        {
            continue;
        }

        vertIdxBufferCopied[vertType] = false;

        // Move all the data to scene's vertex buffer struct
        sceneVerts.vertices = std::move(newSceneVerts.vertices);
        sceneVerts.indices = std::move(newSceneVerts.indices);
        sceneVerts.vertsAllocTracker = std::move(newSceneVerts.vertsAllocTracker);
        sceneVerts.idxsAllocTracker = std::move(newSceneVerts.idxsAllocTracker);

        // Pull references before pushing back
        for (std::pair<const cbe::ObjectPath, MeshVertexView> &meshViewPair : newSceneVerts.meshes)
        {
            auto itr = sceneVerts.meshes.find(meshViewPair.first);
            if (itr != sceneVerts.meshes.cend())
            {
                meshViewPair.second.refs = itr->second.refs;
            }
        }
        sceneVerts.meshes = std::move(newSceneVerts.meshes);
    }

    bVertexUpdating = false;
}

copat::JobSystemFuncAwaiter EngineRenderScene::recreateMaterialBuffers(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    bMaterialsUpdating = true;

    std::unordered_map<String, MaterialShaderParams> newShaderToMaterials;
    {
        RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
        const GlobalRenderingContextBase *rendererContext = renderMan->getGlobalRenderingContext();

        for (std::pair<const String, MaterialShaderParams> &shaderMats : shaderToMaterials)
        {
            debugAssert(shaderMats.second.materialIDToRemove.empty());
            if (shaderMats.second.compIdxToAdd.empty())
            {
                continue;
            }

            MaterialShaderParams &newShaderMats = newShaderToMaterials[shaderMats.first];
            newShaderMats.materialAllocTracker = shaderMats.second.materialAllocTracker;
            newShaderMats.materialRefs = shaderMats.second.materialRefs;
            newShaderMats.materialToIdx = shaderMats.second.materialToIdx;

            newShaderMats.compIdxToAdd = std::move(shaderMats.second.compIdxToAdd);

            uint32 newMatsCount = uint32(Math::toHigherPowOf2(newShaderMats.materialAllocTracker.size() + newShaderMats.compIdxToAdd.size()));
            newShaderMats.materialAllocTracker.resize(newMatsCount);
            newShaderMats.materialRefs.resize(newMatsCount);

            // Vertex type does not matter as material will be same for each vertex
            const PipelineBase *pipeline
                = rendererContext->getDefaultPipeline(shaderMats.first, EVertexType::StaticMesh, ERenderPassFormat::Multibuffer);
            debugAssert(pipeline);

            newShaderMats.shaderParameter = graphicsHelper->createShaderParameters(
                graphicsInstance, pipeline->getParamLayoutAtSet(ShaderParameterUtility::SHADER_UNIQ_SET)
            );
            uint32 newBufferSize = newShaderMats.shaderParameter->getRuntimeBufferRequiredSize(MATERIAL_BUFFER_NAME, newMatsCount);
            newShaderMats.materialData = graphicsHelper->createReadWriteBuffer(graphicsInstance, newBufferSize);

            newShaderMats.shaderParameter->setResourceName(world.getObjectName() + TCHAR("_") + shaderMats.first + TCHAR("_MatsParams"));
            newShaderMats.materialData->setResourceName(world.getObjectName() + TCHAR("_") + shaderMats.first + TCHAR("_MatsBuffer"));

            newShaderMats.materialData->init();
            newShaderMats.shaderParameter->setBufferResource(MATERIAL_BUFFER_NAME, newShaderMats.materialData);
            newShaderMats.shaderParameter->init();
        }
    }

    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::WorkerThreads>{};

    for (std::pair<const String, MaterialShaderParams> &newShaderMats : newShaderToMaterials)
    {
        MaterialShaderParams &shaderMats = shaderToMaterials[newShaderMats.first];

        if (shaderMats.materialAllocTracker.size() != 0)
        {
            // If not 0 count then shader material buffer must be valid
            debugAssert(shaderMats.materialData.isValid() && shaderMats.materialData->isValid());

            BatchCopyBufferInfo copyInfo{
                shaderMats.materialData, newShaderMats.second.materialData,
                CopyBufferInfo{0, 0, uint32(shaderMats.materialData->getResourceSize())}
            };
            // It is okay to directly fill actual shaderMats.materialCopies
            shaderMats.materialCopies.emplace_back(std::move(copyInfo));
        }

        for (SizeT compRenderInfoIdx : newShaderMats.second.compIdxToAdd)
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
            debugAssert(compRenderInfo.materialIndex == 0 && compRenderInfo.shaderName.isEqual(newShaderMats.first));

            auto shaderMatsIdxItr = newShaderMats.second.materialToIdx.find(compRenderInfo.matObjPath);
            if (shaderMatsIdxItr != newShaderMats.second.materialToIdx.end())
            {
                compRenderInfo.materialIndex = vectorIdxToMaterialIdx(shaderMatsIdxItr->second);
                newShaderMats.second.materialRefs[shaderMatsIdxItr->second]++;
            }
            else
            {
                // TODO(Jeslas) : Uncomment below once proper material asset is added
                // cbe::Object *materialInst = cbe::get(compRenderInfo.materialID);
                // if (!cbe::isValid(materialInst))
                //{
                //    debugAssert(!newShaderMats.second.materialToIdx.contains(compRenderInfo.materialID));
                //    continue;
                //}

                SizeT matIdx;
                const bool bAllocated = newShaderMats.second.materialAllocTracker.allocate(1, 1, matIdx);
                fatalAssertf(bAllocated, "Allocation failed(This must never happen unless OOM!)");

                newShaderMats.second.materialToIdx[compRenderInfo.matObjPath] = matIdx;
                newShaderMats.second.materialRefs[matIdx] = 1;

                compRenderInfo.materialIndex = vectorIdxToMaterialIdx(matIdx);
            }
        }
    }

    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::RenderThread>{};

    // Force aborted
    if (!bMaterialsUpdating)
    {
        co_return;
    }

    for (std::pair<const String, MaterialShaderParams> &newShaderMats : newShaderToMaterials)
    {
        // First pull all copies
        SizeT fromIdx = 0, count = 0;
        while (newShaderMats.second.materialAllocTracker.findNextAllocatedBlock(fromIdx, fromIdx, count))
        {
            SizeT endIdx = fromIdx + count;
            for (SizeT matArrayIdx = fromIdx; matArrayIdx != endIdx; ++matArrayIdx)
            {
                createMaterialCopies(newShaderMats.second, vectorIdxToMaterialIdx(matArrayIdx), cmdList, graphicsInstance);
            }
            fromIdx = endIdx;
        }

        MaterialShaderParams &shaderMats = shaderToMaterials[newShaderMats.first];

        shaderMats.bMatsCopied = false;
        shaderMats.materialData = std::move(newShaderMats.second.materialData);
        shaderMats.shaderParameter = std::move(newShaderMats.second.shaderParameter);
        shaderMats.materialAllocTracker = std::move(newShaderMats.second.materialAllocTracker);
        shaderMats.materialRefs = std::move(newShaderMats.second.materialRefs);
        shaderMats.materialToIdx = std::move(newShaderMats.second.materialToIdx);

        if (!newShaderMats.second.materialCopies.empty())
        {
            shaderMats.materialCopies.insert(
                shaderMats.materialCopies.end(), newShaderMats.second.materialCopies.cbegin(), newShaderMats.second.materialCopies.cend()
            );
        }

        if (!newShaderMats.second.hostToMatCopies.empty())
        {
            shaderMats.hostToMatCopies.insert(
                shaderMats.hostToMatCopies.end(), newShaderMats.second.hostToMatCopies.cbegin(), newShaderMats.second.hostToMatCopies.cend()
            );
        }
    }

    bMaterialsUpdating = false;
}

copat::JobSystemFuncAwaiter EngineRenderScene::recreateInstanceBuffers(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    bInstanceParamsUpdating = true;

    InstanceParamsPerVertType newInstancesData[VERTEX_TYPE_COUNT];
    {
        RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
        const GlobalRenderingContextBase *rendererContext = renderMan->getGlobalRenderingContext();

        for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
        {
            InstanceParamsPerVertType &instances = instancesData[vertType];
            InstanceParamsPerVertType &newInstances = newInstancesData[vertType];
            debugAssert(instances.instanceIdxToRemove.empty());

            if (instances.compIdxToAdd.empty())
            {
                continue;
            }

            newInstances.allocTracker = instances.allocTracker;
            newInstances.compIdxToAdd = std::move(instances.compIdxToAdd);

            const uint32 newInstanceCount = uint32(Math::toHigherPowOf2(instances.allocTracker.size() + newInstances.compIdxToAdd.size()));
            newInstances.allocTracker.resize(newInstanceCount);

            // Instance data layout will be unique for each vertex type
            const PipelineBase *pipeline = rendererContext->getDefaultPipeline(
                compsRenderInfo[newInstances.compIdxToAdd.front()].shaderName, EVertexType::Type(vertType), ERenderPassFormat::Multibuffer
            );
            debugAssert(pipeline);

            newInstances.shaderParameter = graphicsHelper->createShaderParameters(
                graphicsInstance, pipeline->getParamLayoutAtSet(ShaderParameterUtility::INSTANCE_UNIQ_SET)
            );
            const uint32 newInstByteSize = newInstances.shaderParameter->getRuntimeBufferRequiredSize(INSTANCES_BUFFER_NAME, newInstanceCount);
            newInstances.instanceData = graphicsHelper->createReadWriteBuffer(graphicsInstance, newInstByteSize);

            newInstances.shaderParameter->setResourceName(
                world.getObjectName() + TCHAR("_")+ EVertexType::toString(EVertexType::Type(vertType)) + TCHAR("_InstParams"));
            newInstances.instanceData->setResourceName(
                world.getObjectName() + TCHAR("_")+ EVertexType::toString(EVertexType::Type(vertType)) + TCHAR("_InstBuffer"));

            newInstances.instanceData->init();
            newInstances.shaderParameter->setBufferResource(INSTANCES_BUFFER_NAME, newInstances.instanceData);
            newInstances.shaderParameter->init();
        }
    }

    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::WorkerThreads>{};

    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        InstanceParamsPerVertType &instances = instancesData[vertType];
        InstanceParamsPerVertType &newInstances = newInstancesData[vertType];

        if (newInstances.compIdxToAdd.empty())
        {
            continue;
        }

        if (instances.allocTracker.size() != 0)
        {
            debugAssert(instances.instanceData.isValid() && instances.instanceData->isValid());

            BatchCopyBufferInfo copyInfo{
                instances.instanceData, newInstances.instanceData, CopyBufferInfo{0, 0, uint32(instances.instanceData->getResourceSize())}
            };
            // It is okay to directly fill actual instances.copies
            instances.copies.emplace_back(std::move(copyInfo));
        }

        for (SizeT compRenderInfoIdx : newInstances.compIdxToAdd)
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
            debugAssert(compRenderInfo.materialIndex != 0 && compRenderInfo.tfIndex == 0);

            SizeT instanceIdx;
            const bool bAllocated = newInstances.allocTracker.allocate(1, 1, instanceIdx);
            debugAssertf(bAllocated, "Allocation failed(This must never happen unless OOM!)");

            compRenderInfo.tfIndex = vectorIdxToInstanceIdx(instanceIdx);
        }
    }

    co_await copat::SwitchJobThreadAwaiter<copat::EJobThreadType::RenderThread>{};

    // Force aborted
    if (!bInstanceParamsUpdating)
    {
        co_return;
    }

    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        InstanceParamsPerVertType &instances = instancesData[vertType];
        InstanceParamsPerVertType &newInstances = newInstancesData[vertType];

        if (newInstances.compIdxToAdd.empty())
        {
            continue;
        }

        // First create all new copies
        for (SizeT compRenderInfoIdx : newInstances.compIdxToAdd)
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
            createInstanceCopies(newInstances, compRenderInfo, cmdList, graphicsInstance);
        }

        // After update instance data has to be copied at least once
        instanceDataCopied[vertType] = false;

        instances.instanceData = std::move(newInstances.instanceData);
        instances.allocTracker = std::move(newInstances.allocTracker);
        instances.shaderParameter = std::move(newInstances.shaderParameter);

        if (!newInstances.copies.empty())
        {
            instances.copies.insert(instances.copies.end(), newInstances.copies.cbegin(), newInstances.copies.cend());
        }
        if (!newInstances.hostToBufferCopies.empty())
        {
            instances.hostToBufferCopies.insert(
                instances.hostToBufferCopies.cend(), newInstances.hostToBufferCopies.cbegin(), newInstances.hostToBufferCopies.cend()
            );
        }
    }

    bInstanceParamsUpdating = false;
}

void EngineRenderScene::createNextDrawList(
    const RenderSceneViewParams &viewParams, IRenderCommandList *, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("CreateDrawList"));
#if DISABLE_PER_FRAME_UPDATE
    static uint32 testCounter = 0;
    static bool testFlag = false;
    if (!bInstanceParamsUpdating && testCounter > 4)
    {
        return;
    }
#endif

    const SizeT totalCompCapacity = compsRenderInfo.totalCount();

    for (std::pair<const String, MaterialShaderParams> &shaderMats : shaderToMaterials)
    {
        for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
        {
            SizeT readDrawListCount = shaderMats.second.cpuDrawListPerVertType[vertType].size();
            shaderMats.second.cpuDrawListPerVertType[vertType].clear();
            shaderMats.second.cpuDrawListPerVertType[vertType].reserve(readDrawListCount);
        }
    }

    std::vector<SizeT> compIndices;
    {
        CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("SetupVisibleComponents"));

        compIndices.reserve(compsVisibility.countOnes());
        for (SizeT i = 0; i != totalCompCapacity; ++i)
        {
            if (!compsVisibility[i])
            {
                continue;
            }
            compIndices.emplace_back(i);
        }

#if DISABLE_PER_FRAME_UPDATE
        if (bInstanceParamsUpdating || bVertexUpdating)
        {
            testFlag = true;
            testCounter = 0;
        }
        if (testFlag)
        {
            testCounter++;
        }
#endif

        std::sort(
            compIndices.begin(), compIndices.end(),
            [this, &viewParams](SizeT lhs, SizeT rhs)
            {
                return (compsRenderInfo[lhs].worldTf.getTranslation() - viewParams.view.translation()).sqrlength()
                       < (compsRenderInfo[rhs].worldTf.getTranslation() - viewParams.view.translation()).sqrlength();
            }
        );
    }
    // Push to cpu draw list buffer
    {
        CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("WriteDrawListBufferCPU"));
        for (SizeT compIdx : compIndices)
        {
            const ComponentRenderInfo &compRenderInfo = compsRenderInfo[compIdx];
            debugAssert(
                compRenderInfo.meshObjPath.isValid() && vertexBuffers[compRenderInfo.vertexType].meshes.contains(compRenderInfo.meshObjPath)
                && compRenderInfo.materialIndex != 0 && compRenderInfo.tfIndex != 0
            );

            MaterialShaderParams &shaderMats = shaderToMaterials[compRenderInfo.shaderName];
            const MeshVertexView &meshView = vertexBuffers[compRenderInfo.vertexType].meshes[compRenderInfo.meshObjPath];
            DrawIndexedIndirectCommand indexedIndirectDraw{ .indexCount = uint32(meshView.idxCount),
                                                            .instanceCount = 1,
                                                            .firstIndex = uint32(meshView.idxOffset),
                                                            .vertexOffset = int32(meshView.vertOffset),
                                                            .firstInstance = uint32(instanceIdxToVectorIdx(compRenderInfo.tfIndex)) };

            shaderMats.cpuDrawListPerVertType[compRenderInfo.vertexType].emplace_back(std::move(indexedIndirectDraw));
        }
    }

    // Now that all cpu draw lists are prepared
    {
        CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("IssueDrawListCopies"));
        for (std::pair<const String, MaterialShaderParams> &shaderMatsPair : shaderToMaterials)
        {
            MaterialShaderParams &shaderMats = shaderMatsPair.second;
            for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
            {
                std::vector<DrawIndexedIndirectCommand> &cpuDrawList = shaderMats.cpuDrawListPerVertType[vertType];
                if (cpuDrawList.empty())
                {
                    continue;
                }

                uint32 drawListIdx = vertType * BUFFER_COUNT + getBufferedWriteOffset();
                // Resize GPU buffer if necessary
                BufferResourceRef bufferRes = shaderMats.drawListPerVertType[drawListIdx];
                shaderMats.drawListCounts[drawListIdx] = uint32(cpuDrawList.size());
                if (!bufferRes.isValid() || !bufferRes->isValid() || bufferRes->bufferCount() < cpuDrawList.size())
                {
                    bufferRes = graphicsHelper->createReadOnlyIndirectBuffer(
                        graphicsInstance, uint32(sizeof(DrawIndexedIndirectCommand)), uint32(cpuDrawList.size())
                    );
                    bufferRes->setResourceName(
                        world.getObjectName()
                        + TCHAR("_") + shaderMatsPair.first + EVertexType::toString(EVertexType::Type(vertType)) + TCHAR("_IdxIndirect"));
                    bufferRes->init();

                    shaderMats.drawListPerVertType[drawListIdx] = bufferRes;
                    shaderMats.drawListCopied[drawListIdx] = false;
                }

                // Now issue copies
                BatchCopyBufferData copyData{ .dst = bufferRes,
                                              .dstOffset = 0,
                                              .dataToCopy = cpuDrawList.data(),
                                              .size = uint32(bufferRes->bufferStride() * cpuDrawList.size()) };
                shaderMats.drawListCopies.emplace_back(copyData);
            }
        }
    }
}

void EngineRenderScene::renderTheSceneRenderThread(
    const RenderSceneViewParams &viewParams, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
    const GraphicsHelperAPI * /*graphicsHelper*/
)
{
    IRenderInterfaceModule *renderModule = IRenderInterfaceModule::get();
    RenderManager *renderMan = renderModule->getRenderManager();

    // TODO(Jeslas) : Update bindless set here

    // Update scene view
    ViewData viewData{ .projection = viewParams.view.projectionMatrix() };
    viewData.view = viewParams.view.viewMatrix(viewData.invView);
    viewData.invProjection = viewData.projection.inverse();
    viewData.w2clip = viewData.projection * viewData.invView;
    sceneViewParams.peek(1)->setBuffer(RenderSceneBase::VIEW_PARAM_NAME, viewData);

    uint32 bufferedReadOffset = getBufferedReadOffset(), bufferedWriteOffset = getBufferedWriteOffset();
    // Mark edited buffers that needs to be changed queues
    std::unordered_map<MemoryResourceRef, EQueueFunction> transferReleases{
        {sceneViewParams.peek(1)->getBufferResource(RenderSceneBase::VIEW_PARAM_NAME), EQueueFunction::Transfer}
    };
    std::vector<ShaderParametersRef> resBarriers;
    std::vector<BufferResourceRef> vertexBarriers;
    std::vector<BufferResourceRef> indexBarriers;
    std::vector<BufferResourceRef> indirectDrawBarriers;
    resBarriers.reserve(VERTEX_TYPE_COUNT + shaderToMaterials.size() + 2);
    vertexBarriers.reserve(VERTEX_TYPE_COUNT);
    indexBarriers.reserve(VERTEX_TYPE_COUNT);
    indirectDrawBarriers.reserve(VERTEX_TYPE_COUNT * shaderToMaterials.size());
    for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
    {
        if (!vertexBuffers[vertType].copies.empty())
        {
            transferReleases[vertexBuffers[vertType].indices] = EQueueFunction::Transfer;
            transferReleases[vertexBuffers[vertType].vertices] = EQueueFunction::Transfer;
        }

        if (!(instancesData[vertType].copies.empty() && instancesData[vertType].hostToBufferCopies.empty()))
        {
            transferReleases[instancesData[vertType].instanceData] = EQueueFunction::Transfer;
        }

        // Add to barrier this read
        if (instancesData[vertType].shaderParameter && instanceDataCopied[vertType])
        {
            resBarriers.emplace_back(instancesData[vertType].shaderParameter);
        }
        if (vertIdxBufferCopied[vertType] && vertexBuffers[vertType].vertices && vertexBuffers[vertType].indices)
        {
            vertexBarriers.emplace_back(vertexBuffers[vertType].vertices);
            indexBarriers.emplace_back(vertexBuffers[vertType].indices);
        }
    }

    bool bHasAnyDraws = false;
    for (const std::pair<const String, MaterialShaderParams> &shaderMats : shaderToMaterials)
    {
        if (!(shaderMats.second.materialCopies.empty() && shaderMats.second.hostToMatCopies.empty()))
        {
            transferReleases[shaderMats.second.materialData] = EQueueFunction::Transfer;
        }

        if (!shaderMats.second.drawListCopies.empty())
        {
            for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
            {
                transferReleases[shaderMats.second.drawListPerVertType[vertType * BUFFER_COUNT + bufferedWriteOffset]]
                    = EQueueFunction::Transfer;
            }
        }

        // To barrier this read
        bool bHasAnyMatDraws = false;
        for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
        {
            uint32 drawListIdx = vertType * BUFFER_COUNT + bufferedReadOffset;
            if (shaderMats.second.drawListCounts[drawListIdx] != 0 && shaderMats.second.drawListCopied[drawListIdx])
            {
                bHasAnyMatDraws = true;
                indirectDrawBarriers.emplace_back(shaderMats.second.drawListPerVertType[drawListIdx]);
            }
        }
        if (bHasAnyMatDraws && shaderMats.second.bMatsCopied && shaderMats.second.shaderParameter)
        {
            bHasAnyDraws = true;
            resBarriers.emplace_back(shaderMats.second.shaderParameter);
        }
    }

    ShaderParametersRef frameBindlessParam = bindlessSet.pop();
    ShaderParametersRef frameSceneViewParam = sceneViewParams.pop();
    ShaderParametersRef frameColorResolveParam = colorResolveParams.pop();
    ShaderParametersRef frameDepthResolveParam = depthResolveParams.pop();

    resBarriers.emplace_back(frameBindlessParam);
    resBarriers.emplace_back(frameSceneViewParam);

    // For now not supporting MSAA
    debugAssert(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get() == EPixelSampleCount::SampleCount1);

    frameTextures[ERendererIntermTexture::GBufferDiffuse]
        = rtPool.getTexture(cmdList, ERendererIntermTexture::GBufferDiffuse, viewParams.viewportSize, {});
    frameTextures[ERendererIntermTexture::GBufferNormal]
        = rtPool.getTexture(cmdList, ERendererIntermTexture::GBufferNormal, viewParams.viewportSize, {});
    frameTextures[ERendererIntermTexture::GBufferARM]
        = rtPool.getTexture(cmdList, ERendererIntermTexture::GBufferARM, viewParams.viewportSize, {});
    frameTextures[ERendererIntermTexture::GBufferDepth]
        = rtPool.getTexture(cmdList, ERendererIntermTexture::GBufferDepth, viewParams.viewportSize, {});

    // TODO(Jeslas) : Support depth view may be?
    frameColorResolveParam->setTextureParam(
        STRID("quadTexture"), frameTextures[ERendererIntermTexture::GBufferDiffuse].renderResource(), GlobalBuffers::linearSampler()
    );
    frameColorResolveParam->updateParams(cmdList, graphicsInstance);

    IRect viewport{
        {                        0,                         0},
        {viewParams.viewportSize.x, viewParams.viewportSize.y}
    };
    IRect scissor{
        {                        0,                         0},
        {viewParams.viewportSize.x, viewParams.viewportSize.y}
    };
    RenderPassClearValue clearVal;
    clearVal.colors = { LinearColorConst::BLACK, LinearColorConst::BLACK, LinearColorConst::BLACK };

    const GraphicsResource *cmdBuffer = cmdList->startCmd(getCmdBufferName(), EQueueFunction::Graphics, true);
    SCOPED_CMD_MARKER(cmdList, cmdBuffer, RenderingScene);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ToGBuffer);

        const IRenderTargetTexture *gbufferRts[]
            = { &frameTextures[ERendererIntermTexture::GBufferDiffuse], &frameTextures[ERendererIntermTexture::GBufferNormal],
                &frameTextures[ERendererIntermTexture::GBufferARM], &frameTextures[ERendererIntermTexture::GBufferDepth] };

        LocalPipelineContext defaultPipelineCntxt;
        defaultPipelineCntxt.materialName = TCHAR("Default");
        defaultPipelineCntxt.forVertexType = EVertexType::StaticMesh;
        defaultPipelineCntxt.renderpassFormat = ERenderPassFormat::Multibuffer;
        renderMan->preparePipelineContext(&defaultPipelineCntxt, gbufferRts);

        if (bHasAnyDraws)
        {
            CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("IssueBarriers"));
            cmdList->cmdBarrierResources(cmdBuffer, resBarriers);
            cmdList->cmdBarrierVertices(cmdBuffer, vertexBarriers);
            cmdList->cmdBarrierIndices(cmdBuffer, indexBarriers);
            cmdList->cmdBarrierIndirectDraws(cmdBuffer, indirectDrawBarriers);
        }

        RenderPassAdditionalProps additionalProps{ .bAllowUndefinedLayout = true };
        SCOPED_RENDERPASS(cmdList, cmdBuffer, defaultPipelineCntxt, viewport, additionalProps, clearVal, ToGBuffer);
        if (bHasAnyDraws)
        {
            // This has to be upside down along y
            IRect drawViewport{
                {                        0, viewParams.viewportSize.y},
                {viewParams.viewportSize.x,                         0}
            };
            cmdList->cmdSetViewportAndScissor(cmdBuffer, drawViewport, scissor);

            ShaderParametersRef commonDescSets[] = { frameBindlessParam, frameSceneViewParam };
            cmdList->cmdBindDescriptorsSets(cmdBuffer, defaultPipelineCntxt, commonDescSets);

            GraphicsPipelineState pipelineState;
            pipelineState.pipelineQuery.drawMode = EPolygonDrawMode::Fill;
            pipelineState.pipelineQuery.cullingMode = ECullingMode::BackFace;
            for (const std::pair<const String, MaterialShaderParams> &shaderMats : shaderToMaterials)
            {
                LocalPipelineContext pipelineCntxt;
                pipelineCntxt.materialName = shaderMats.first;
                pipelineCntxt.renderpassFormat = ERenderPassFormat::Multibuffer;
                pipelineCntxt.forVertexType = EVertexType::StaticMesh;
                renderMan->preparePipelineContext(&pipelineCntxt, gbufferRts);
                cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineCntxt, { &shaderMats.second.shaderParameter, 1 });
                for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
                {
                    uint32 drawListIdx = vertType * BUFFER_COUNT + bufferedReadOffset;
                    if (shaderMats.second.drawListCounts[drawListIdx] == 0 || !instancesData[vertType].shaderParameter.isValid()
                        || instancesData[vertType].instanceData->bufferCount() == 0)
                    {
                        continue;
                    }

                    pipelineCntxt.forVertexType = EVertexType::Type(vertType);
                    renderMan->preparePipelineContext(&pipelineCntxt, gbufferRts);

                    cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineCntxt, { &instancesData[vertType].shaderParameter, 1 });
                    cmdList->cmdBindGraphicsPipeline(cmdBuffer, pipelineCntxt, pipelineState);
                    cmdList->cmdBindVertexBuffer(cmdBuffer, 0, vertexBuffers[vertType].vertices, 0);
                    cmdList->cmdBindIndexBuffer(cmdBuffer, vertexBuffers[vertType].indices, 0);

                    static_assert(
                        std::is_same_v<
                            std::remove_reference_t<decltype(std::declval<decltype(MaterialShaderParams::cpuDrawListPerVertType)>()[0]
                            )>::value_type,
                            DrawIndexedIndirectCommand>,
                        "Fix me! Indexed indirect command struct mismatch!"
                    );
                    cmdList->cmdDrawIndexedIndirect(
                        cmdBuffer, shaderMats.second.drawListPerVertType[drawListIdx], 0, shaderMats.second.drawListCounts[drawListIdx],
                        sizeof(DrawIndexedIndirectCommand)
                    );
                }
            }
        }
    }
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, ResolveFinalColor);

        LocalPipelineContext pipelineCntxt;
        pipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
        pipelineCntxt.materialName = TCHAR("DrawQuadFromTexture");
        const IRenderTargetTexture *rtPtr = &frameTextures[ERendererIntermTexture::FinalColor];
        renderModule->getRenderManager()->preparePipelineContext(&pipelineCntxt, { &rtPtr, 1 });

        GraphicsPipelineState pipelineState;
        pipelineState.pipelineQuery.drawMode = EPolygonDrawMode::Fill;
        pipelineState.pipelineQuery.cullingMode = ECullingMode::BackFace;

        RenderPassAdditionalProps additionalProps;
        additionalProps.bAllowUndefinedLayout = true;

        cmdList->cmdBeginRenderPass(cmdBuffer, pipelineCntxt, viewport, additionalProps, clearVal);

        cmdList->cmdBindGraphicsPipeline(cmdBuffer, pipelineCntxt, pipelineState);

        cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineCntxt, frameColorResolveParam);

        cmdList->cmdDrawVertices(cmdBuffer, 0, 3);

        cmdList->cmdEndRenderPass(cmdBuffer);
    }
    cmdList->cmdReleaseQueueResources(cmdBuffer, EQueueFunction::Graphics, transferReleases);
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo2 submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, submitInfo);

    bindlessSet.push(frameBindlessParam);
    sceneViewParams.push(frameSceneViewParam);
    colorResolveParams.push(frameColorResolveParam);
    depthResolveParams.push(frameDepthResolveParam);
}
