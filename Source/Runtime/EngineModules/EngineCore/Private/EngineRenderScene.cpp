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
#include "Types/Camera/Camera.h"
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
#include "IRenderInterfaceModule.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"

namespace ERendererIntermTexture
{
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

const RendererIntermTexture &SceneRenderTexturePool::getTexture(
    IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size3D size, PoolTextureDesc textureDesc
)
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
        "SceneRenderTexturePool", "Allocated new RT %s(%d, %d, %d) under type %s", texture.renderTargetResource()->getResourceName(),
        texture.rtTexture->getImageSize().x, texture.rtTexture->getImageSize().y, texture.rtTexture->getImageSize().z,
        ERendererIntermTexture::toString(rtType)
    );

    // Insert into pool
    poolTextures[rtType].emplace(std::piecewise_construct, std::forward_as_tuple(size), std::forward_as_tuple(idx));
    return texture;
}

const RendererIntermTexture &SceneRenderTexturePool::getTexture(
    IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size2D size, PoolTextureDesc textureDesc
)
{
    ASSERT_INSIDE_RENDERTHREAD();
    return getTexture(cmdList, rtType, Size3D(size, 1), textureDesc);
}

const RendererIntermTexture *SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size3D size)
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

        if (!cmdList->hasCmdsUsingResource(intermTexture.renderTargetResource())
            && (intermTexture.renderTargetResource() == intermTexture.renderResource()
                || !cmdList->hasCmdsUsingResource(intermTexture.renderResource())))
        {
            return &intermTexture;
        }
    }
    return nullptr;
}

const RendererIntermTexture *SceneRenderTexturePool::getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size2D size)
{
    ASSERT_INSIDE_RENDERTHREAD();
    return getTexture(cmdList, rtType, Size3D(size, 1));
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

            if (!cmdList->hasCmdsUsingResource(intermTexture.renderTargetResource())
                && (intermTexture.renderTargetResource() == intermTexture.renderResource()
                    || !cmdList->hasCmdsUsingResource(intermTexture.renderResource())))
            {
                safeToDeleteRts.emplace_back(intermTexture.renderTargetResource());
                if (intermTexture.renderTargetResource() != intermTexture.renderResource())
                {
                    safeToDeleteRts.emplace_back(intermTexture.renderResource());
                }

                LOG_VERBOSE(
                    "SceneRenderTexturePool", "Clearing Texture %s(%d, %d, %d) from type %s",
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

const RendererIntermTexture &EngineRenderScene::getFinalColor(IRenderCommandList *cmdList, Short2D size)
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
        for (cbe::TransformComponent *tfComp : actor->getTransformComponents())
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(tfComp))
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
            performTransferCopies(cmdList, graphicsInstance, graphicsHelper);
        }
    );

    // No need to clear the bindings as EngineRenderScene lifetime is less than that of World itself
    inWorld->onTfCompAdded.bindLambda(
        [this](cbe::Object *compObj)
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(compObj))
            {
                componentUpdates.compsAdded.emplace_back(renderComp);
            }
        }
    );
    inWorld->onTfCompRemoved.bindLambda(
        [this](cbe::Object *compObj)
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(compObj))
            {
                componentUpdates.compsRemoved.emplace_back(renderComp->getStringID());
                // Remove component from added components list if both happening in same frame
                std::erase(componentUpdates.compsAdded, compObj);
                std::erase(componentUpdates.recreateComps, compObj);
            }
        }
    );
    inWorld->onTfCompInvalidated.bindLambda(
        [this](cbe::Object *compObj)
        {
            if (cbe::RenderableComponent *renderComp = cbe::cast<cbe::RenderableComponent>(compObj))
            {
                componentUpdates.recreateComps.emplace_back(renderComp);
            }
        }
    );
    inWorld->onTfCompTransformed.bindLambda(
        [this](cbe::Object *compObj)
        {
            if (cbe::TransformComponent *renderComp = cbe::cast<cbe::TransformComponent>(compObj))
            {
                componentUpdates.compsTransformed.emplace_back(renderComp);
            }
        }
    );
}

void EngineRenderScene::clearScene()
{
    RenderThreadEnqueuer::execInRenderThreadAndWait(
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
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

    // TODO(Jeslas) : Clear scene
}

void EngineRenderScene::renderTheScene(Short2D viewportSize, const Camera &viewCamera)
{
    // start the rendering in Renderer
    ENQUEUE_RENDER_COMMAND(RenderScene)
    (
        [viewCamera, viewportSize, compUpdates = std::move(componentUpdates),
         this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            syncWorldCompsRenderThread(compUpdates, cmdList, graphicsInstance, graphicsHelper);
            updateVisibility(viewCamera);

            createNextDrawList(viewCamera, cmdList, graphicsInstance, graphicsHelper);

            cmdList->finishCmd(getCmdBufferName());
            lastRT = getFinalColor(cmdList, viewportSize);
            renderTheSceneRenderThread(viewportSize, viewCamera, cmdList, graphicsInstance, graphicsHelper);
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
    return &lastRT;
}

void EngineRenderScene::onLastRTCopied()
{
    // TODO(Jeslas) : OnLastRTCopied, Is this needed?
}

FORCE_INLINE void EngineRenderScene::addMeshRef(EVertexType::Type vertType, StringID meshID, SizeT compRenderInfoIdx)
{
    VerticesPerVertType &sceneVerts = vertexBuffers[vertType];
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];

    auto meshItr = sceneVerts.meshes.find(meshID);
    if (meshItr != sceneVerts.meshes.end())
    {
        meshItr->second.refs++;
    }
    else if (bVertexUpdating)
    {
        sceneVerts.meshesToAdd.emplace_back(meshID, compRenderInfoIdx);
    }
    else
    {
        debugAssert(
            compRenderInfo.cpuIdxBuffer.isValid() && compRenderInfo.cpuVertBuffer.isValid() && compRenderInfo.cpuIdxBuffer->isValid()
            && compRenderInfo.cpuVertBuffer->isValid() && compRenderInfo.cpuVertBuffer->bufferStride() > 1
            && compRenderInfo.cpuIdxBuffer->bufferStride() > 1
        );
        cbe::Object *mesh = cbe::get(meshID);
        if (!cbe::isValid(mesh))
        {
            debugAssert(!sceneVerts.meshes.contains(meshID));
            return;
        }

        SizeT idxOffset, vertOffset;
        bool bIdxAlloced = sceneVerts.idxsAllocTracker.allocate(compRenderInfo.cpuIdxBuffer->bufferCount(), 1, idxOffset);
        bool bVertAlloced = sceneVerts.vertsAllocTracker.allocate(compRenderInfo.cpuVertBuffer->bufferCount(), 1, vertOffset);
        if (bIdxAlloced && bVertAlloced)
        {
            MeshVertexView &vertView = sceneVerts.meshes[meshID];
            vertView.idxOffset = idxOffset;
            vertView.idxCount = compRenderInfo.cpuIdxBuffer->bufferCount();
            vertView.vertOffset = vertOffset;
            vertView.vertCount = compRenderInfo.cpuVertBuffer->bufferCount();
            vertView.refs = 1;

            uint32 vertStride = compRenderInfo.cpuVertBuffer->bufferStride(), idxStride = compRenderInfo.cpuIdxBuffer->bufferStride();

            BatchCopyBufferInfo copyInfo;
            copyInfo.dst = sceneVerts.vertices;
            copyInfo.src = compRenderInfo.cpuVertBuffer;
            copyInfo.copyInfo = { 0, vertView.vertOffset * vertStride, uint32(vertView.vertCount * vertStride) };
            sceneVerts.copies.emplace_back(std::move(copyInfo));

            copyInfo.dst = sceneVerts.indices;
            copyInfo.src = compRenderInfo.cpuIdxBuffer;
            copyInfo.copyInfo = { 0, vertView.idxOffset * idxStride, uint32(vertView.idxCount * idxStride) };
            sceneVerts.copies.emplace_back(std::move(copyInfo));
        }
        else
        {
            // Not enough space allocate new buffers and copy
            sceneVerts.meshesToAdd.emplace_back(meshID, compRenderInfoIdx);
        }
    }
}

FORCE_INLINE void EngineRenderScene::removeMeshRef(EVertexType::Type vertType, MeshVertexView &meshVertView, StringID meshID)
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
    MaterialShaderParams &shaderMats, const ComponentRenderInfo &compRenderInfo, IRenderCommandList *cmdList,
    IGraphicsInstance *graphicsInstance
) const
{
    // TODO(Jeslas) : This is test code, must be unique per shader
    debugAssert(shaderMats.shaderParameter.isValid() && shaderMats.materialData.isValid());
    StringID paramPath[] = { MATERIAL_BUFFER_NAME, STRID("meshData"), STRID("meshColor") };
    uint32 indices[] = { 0, uint32(materialIdxToVectorIdx(compRenderInfo.materialIndex)), 0 };
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
    auto shaderMatsIdxItr = shaderMats.materialToIdx.find(compRenderInfo.materialID);
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
            shaderMats.materialToIdx[compRenderInfo.materialID] = matIdx;
            shaderMats.materialRefs[matIdx] = 1;

            compRenderInfo.materialIndex = vectorIdxToMaterialIdx(matIdx);
            IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get();
            createMaterialCopies(
                shaderMats, compRenderInfo, renderInterface->getRenderManager()->getRenderCmds(), renderInterface->currentGraphicsInstance()
            );
        }
        else
        {
            // Not enough space allocate new buffers and copy
            shaderMats.compIdxToAdd.emplace_back(compRenderInfoIdx);
        }
    }
}

FORCE_INLINE void EngineRenderScene::removeMaterialAt(SizeT matVectorIdx, StringID materialID, MaterialShaderParams &shaderMats)
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
        auto compToIdxItr = componentToRenderInfo.find(compToAdd->getStringID());
        if (compToIdxItr == componentToRenderInfo.end())
        {
            SizeT idx = compsRenderInfo.get();
            createRenderInfo(compToAdd, idx);
            if (compsRenderInfo[idx].meshID.isValid())
            {
                debugAssert(compsRenderInfo[idx].cpuVertBuffer.isValid() && compsRenderInfo[idx].cpuIdxBuffer.isValid());
                addMeshRef(compsRenderInfo[idx].vertexType, compsRenderInfo[idx].meshID, idx);
            }
            componentToRenderInfo[compToAdd->getStringID()] = idx;
        }
    }
}

void EngineRenderScene::removeRenderComponents(const std::vector<StringID> &renderComps)
{
    for (StringID compToRemove : renderComps)
    {
        auto compToIdxItr = componentToRenderInfo.find(compToRemove);
        if (compToIdxItr != componentToRenderInfo.end())
        {
            SizeT idx = compToIdxItr->second;
            StringID currMesh = compsRenderInfo[idx].meshID;
            EVertexType::Type currVertType = compsRenderInfo[idx].vertexType;

            if (currMesh.isValid())
            {
                removeMeshRef(currVertType, vertexBuffers[currVertType].meshes[currMesh], currMesh);
            }
            cbe::RenderableComponent *comp = cbe::cast<cbe::RenderableComponent>(cbe::get(compToRemove));
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
        auto compToIdxItr = componentToRenderInfo.find(compToRecreate->getStringID());
        if (compToIdxItr == componentToRenderInfo.end())
        {
            SizeT idx = compsRenderInfo.get();
            createRenderInfo(compToRecreate, idx);

            if (compsRenderInfo[idx].meshID.isValid())
            {
                debugAssert(compsRenderInfo[idx].cpuVertBuffer.isValid() && compsRenderInfo[idx].cpuIdxBuffer.isValid());
                addMeshRef(compsRenderInfo[idx].vertexType, compsRenderInfo[idx].meshID, idx);
            }
            componentToRenderInfo[compToRecreate->getStringID()] = idx;
        }
        else
        {
            SizeT idx = compToIdxItr->second;
            StringID currMesh = compsRenderInfo[idx].meshID;
            EVertexType::Type currVertType = compsRenderInfo[idx].vertexType;

            destroyRenderInfo(compToRecreate, idx);
            createRenderInfo(compToRecreate, idx);

            StringID newMesh = compsRenderInfo[idx].meshID;
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
    const std::vector<cbe::TransformComponent *> &comps, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance
)
{
    for (cbe::TransformComponent *updateTf : comps)
    {
        if (!cbe::isValid(updateTf))
        {
            continue;
        }

        auto compRenderIdxItr = componentToRenderInfo.find(updateTf->getStringID());
        if (compRenderIdxItr != componentToRenderInfo.cend())
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderIdxItr->second];
            // TODO(Jeslas) : Getting world tf here is safe?
            compRenderInfo.worldTf = updateTf->getWorldTransform();

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
    compRenderInfo.compID = comp->getStringID();
    comp->setupRenderInfo(compRenderInfo);

    // TODO(Jeslas): Remove below demo code
    compRenderInfo.shaderName = TCHAR("SingleColor");
    addCompMaterialData(compRenderInfoIdx);
    addCompInstanceData(compRenderInfoIdx);
}

void EngineRenderScene::destroyRenderInfo(const cbe::RenderableComponent *comp, SizeT compRenderInfoIdx)
{
    ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];

    if (compRenderInfo.materialIndex != 0)
    {
        removeMaterialAt(
            materialIdxToVectorIdx(compRenderInfo.materialIndex), compRenderInfo.materialID, shaderToMaterials[compRenderInfo.shaderName]
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

void EngineRenderScene::performTransferCopies(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
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
                allHostToDeviceCopies.insert(
                    allHostToDeviceCopies.end(), shaderMaterials.second.drawListCopies.cbegin(), shaderMaterials.second.drawListCopies.cend()
                );
                allCopies.insert(allCopies.end(), shaderMaterials.second.materialCopies.cbegin(), shaderMaterials.second.materialCopies.cend());
                allHostToDeviceCopies.insert(
                    allHostToDeviceCopies.end(), shaderMaterials.second.hostToMatCopies.cbegin(), shaderMaterials.second.hostToMatCopies.cend()
                );
                shaderMaterials.second.drawListCopies.clear();
                shaderMaterials.second.materialCopies.clear();
                shaderMaterials.second.hostToMatCopies.clear();
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
    }
    cmdList->cmdReleaseQueueResources(cmdBuffer, EQueueFunction::Graphics);
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo2 submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, submitInfo);
}

void EngineRenderScene::updateVisibility(const Camera &viewCamera)
{
    const SizeT totalCompCapacity = compsRenderInfo.totalCount();
    compsVisibility.resize(totalCompCapacity);
    compsVisibility.resetRange(0, totalCompCapacity);

    ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();
    // Will be inside frustum only if every other condition to render a mesh is valid
    std::vector<bool, CBEStrStackAllocatorExclusive<bool>> compsInsideFrustum(totalCompCapacity, false, appInstance->getRenderFrameAllocator());

    Matrix4 w2clip = viewCamera.projectionMatrix() * viewCamera.viewMatrix();

    copat::waitOnAwaitable(copat::dispatch(
        appInstance->jobSystem,
        copat::DispatchFunctionType::createLambda(
            [this, &compsInsideFrustum, &w2clip](uint32 idx)
            {
                if (!compsRenderInfo.isValid(idx))
                {
                    return;
                }

                const ComponentRenderInfo &compRenderInfo = compsRenderInfo[idx];
                cbe::RenderableComponent *renderComp
                    = compRenderInfo.meshID.isValid() ? cbe::cast<cbe::RenderableComponent>(cbe::get(compRenderInfo.compID)) : nullptr;
                if (vertexBuffers[compRenderInfo.vertexType].meshes.contains(compRenderInfo.meshID) && compRenderInfo.tfIndex != 0
                    && compRenderInfo.materialIndex != 0 && renderComp != nullptr)
                {
                    Matrix4 obj2Clip = w2clip * compRenderInfo.worldTf.getTransformMatrix();
                    AABB localBound = renderComp->getLocalBound();
                    if (!localBound.isValidAABB())
                    {
                        return;
                    }

                    Vector3D aabbCorners[8];
                    localBound.boundCorners(aabbCorners);

                    for (uint32 i = 0; i != ARRAY_LENGTH(aabbCorners); ++i)
                    {
                        Vector4D projectedPt = obj2Clip * Vector4D(aabbCorners[i], 1.0f);
                        projectedPt.x() = Math::abs(projectedPt.x());
                        projectedPt.y() = Math::abs(projectedPt.y());
                        projectedPt.z() /= projectedPt.w();
                        projectedPt.w() = Math::abs(projectedPt.w());
                        if (projectedPt.x() <= projectedPt.w() && projectedPt.y() <= projectedPt.w() && projectedPt.z() > 0
                            && projectedPt.z() <= 1)
                        {
                            compsInsideFrustum[idx] = true;
                            return;
                        }
                    }
                }
            }
        ),
        totalCompCapacity
    ));

    for (SizeT i = 0; i != totalCompCapacity; ++i)
    {
        if (compsInsideFrustum[i])
        {
            compsVisibility[i] = true;
        }
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
                for (StringID meshToRemove : meshesToRemove)
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
                for (const std::pair<const StringID, SizeT> &meshToAdd : meshesToAdd)
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
                for (StringID matIDToRemove : matIDsToRemove)
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

copat::NormalFuncAwaiter EngineRenderScene::recreateSceneVertexBuffers(
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
        for (const std::pair<const StringID, SizeT> &meshToAdd : newBuffers[vertType].meshesToAdd)
        {
            const ComponentRenderInfo &compRenderInfo = compsRenderInfo[newSceneVerts.meshesToAdd[0].second];
            addVertsCount += compRenderInfo.cpuVertBuffer->bufferCount();
            addIdxsCount += compRenderInfo.cpuIdxBuffer->bufferCount();
        }

        // Setup newSceneVerts data for new size and counts
        uint64 newVertsCount = sceneVerts.vertsAllocTracker.size() + addVertsCount,
               newIdxsCount = sceneVerts.idxsAllocTracker.size() + addIdxsCount;
        newVertsCount = Math::toHigherPowOf2(newVertsCount);
        newIdxsCount = Math::toHigherPowOf2(newIdxsCount);

        newSceneVerts.vertsAllocTracker.resize(newVertsCount);
        newSceneVerts.idxsAllocTracker.resize(newIdxsCount);

        newSceneVerts.meshes.reserve(sceneVerts.meshes.size() + newSceneVerts.meshesToAdd.size());

        newSceneVerts.vertices = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, vertexStride, newVertsCount);
        newSceneVerts.vertices->setResourceName(world.getObjectName() + TCHAR("_") + EVertexType::toString(EVertexType::Type(vertType)));
        newSceneVerts.vertices->init();

        newSceneVerts.indices = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, idxStride, newIdxsCount);
        newSceneVerts.indices->setResourceName(world.getObjectName() + TCHAR("_Indices"));
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
        for (const std::pair<const StringID, MeshVertexView> &meshViewPair : sceneVerts.meshes)
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

        for (const std::pair<const StringID, SizeT> &meshToAdd : newBuffers[vertType].meshesToAdd)
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
        // Move all the data to scene's vertex buffer struct
        sceneVerts.vertices = std::move(newSceneVerts.vertices);
        sceneVerts.indices = std::move(newSceneVerts.indices);
        sceneVerts.vertsAllocTracker = std::move(newSceneVerts.vertsAllocTracker);
        sceneVerts.idxsAllocTracker = std::move(newSceneVerts.idxsAllocTracker);

        // Pull references before pushing back
        for (std::pair<const StringID, MeshVertexView> &meshViewPair : newSceneVerts.meshes)
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

copat::NormalFuncAwaiter EngineRenderScene::recreateMaterialBuffers(
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

        for (uint32 compRenderInfoIdx : newShaderMats.second.compIdxToAdd)
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
            debugAssert(compRenderInfo.materialIndex == 0 && compRenderInfo.shaderName.isEqual(newShaderMats.first));

            auto shaderMatsIdxItr = newShaderMats.second.materialToIdx.find(compRenderInfo.materialID);
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
                debugAssertf(bAllocated, "Allocation failed(This must never happen unless OOM!)");

                newShaderMats.second.materialToIdx[compRenderInfo.materialID] = matIdx;
                newShaderMats.second.materialRefs[matIdx] = 1;

                compRenderInfo.materialIndex = vectorIdxToMaterialIdx(matIdx);
                createMaterialCopies(newShaderMats.second, compRenderInfo, cmdList, graphicsInstance);
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
        MaterialShaderParams &shaderMats = shaderToMaterials[newShaderMats.first];

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

copat::NormalFuncAwaiter EngineRenderScene::recreateInstanceBuffers(
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

        for (uint32 compRenderInfoIdx : newInstances.compIdxToAdd)
        {
            ComponentRenderInfo &compRenderInfo = compsRenderInfo[compRenderInfoIdx];
            debugAssert(compRenderInfo.materialIndex != 0 && compRenderInfo.tfIndex == 0);

            SizeT instanceIdx;
            const bool bAllocated = newInstances.allocTracker.allocate(1, 1, instanceIdx);
            debugAssertf(bAllocated, "Allocation failed(This must never happen unless OOM!)");

            compRenderInfo.tfIndex = vectorIdxToInstanceIdx(instanceIdx);
            createInstanceCopies(newInstances, compRenderInfo, cmdList, graphicsInstance);
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
    const Camera &viewCamera, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    const SizeT totalCompCapacity = compsRenderInfo.totalCount();
    const uint32 drawListWriteOffset = getDrawListWriteOffset();

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
    compIndices.reserve(compsVisibility.countOnes());
    for (SizeT i = 0; i != totalCompCapacity; ++i)
    {
        if (!compsVisibility[i])
        {
            continue;
        }
        compIndices.emplace_back(i);
    }
    std::sort(
        compIndices.begin(), compIndices.end(),
        [this, &viewCamera](SizeT lhs, SizeT rhs)
        {
            return (compsRenderInfo[lhs].worldTf.getTranslation() - viewCamera.translation()).sqrlength()
                   < (compsRenderInfo[rhs].worldTf.getTranslation() - viewCamera.translation()).sqrlength();
        }
    );
    for (SizeT compIdx : compIndices)
    {
        const ComponentRenderInfo &compRenderInfo = compsRenderInfo[compIdx];
        debugAssert(
            compRenderInfo.meshID.isValid() && vertexBuffers[compRenderInfo.vertexType].meshes.contains(compRenderInfo.meshID)
            && compRenderInfo.materialIndex != 0 && compRenderInfo.tfIndex != 0
        );

        MaterialShaderParams &shaderMats = shaderToMaterials[compRenderInfo.shaderName];
        const MeshVertexView &meshView = vertexBuffers[compRenderInfo.vertexType].meshes[compRenderInfo.meshID];
        DrawIndexedIndirectCommand indexedIndirectDraw{ .indexCount = uint32(meshView.idxCount),
                                                        .instanceCount = 1,
                                                        .firstIndex = uint32(meshView.idxOffset),
                                                        .vertexOffset = int32(meshView.vertOffset),
                                                        .firstInstance = uint32(compRenderInfo.tfIndex) };

        shaderMats.cpuDrawListPerVertType[compRenderInfo.vertexType].emplace_back(std::move(indexedIndirectDraw));
    }

    // Now that all cpu draw lists are prepared now sort and merge
    // Right now sorting and merging is not much worth it, However after modifying recreateSceneVerts to keeps instances of same mesh together
    // this will improve
    for (std::pair<const String, MaterialShaderParams> &shaderMatsPair : shaderToMaterials)
    {
        MaterialShaderParams &shaderMats = shaderMatsPair.second;
        for (uint32 vertType = EVertexType::TypeStart; vertType != EVertexType::TypeEnd; ++vertType)
        {
            uint32 drawListIdx = vertType * 2 + drawListWriteOffset;
            std::vector<DrawIndexedIndirectCommand> &cpuDrawList = shaderMats.cpuDrawListPerVertType[vertType];
            std::sort(
                cpuDrawList.begin(), cpuDrawList.end(),
                [](const DrawIndexedIndirectCommand &lhs, const DrawIndexedIndirectCommand &rhs)
                {
                    return lhs.firstInstance == rhs.firstInstance
                               ? lhs.vertexOffset == rhs.vertexOffset ? lhs.firstIndex < rhs.firstIndex : lhs.vertexOffset < rhs.vertexOffset
                               : lhs.firstInstance < rhs.firstInstance;
                }
            );

            for (SizeT i = 0; i != cpuDrawList.size(); ++i)
            {
                SizeT nextIdx = i + 1;
                uint32 nextInstanceExp = cpuDrawList[i].firstInstance + 1;
                while (nextIdx != cpuDrawList.size() && cpuDrawList[nextIdx].firstInstance == nextInstanceExp
                       && cpuDrawList[nextIdx].vertexOffset == cpuDrawList[i].vertexOffset
                       && cpuDrawList[nextIdx].firstIndex == cpuDrawList[i].firstIndex)
                {
                    nextIdx++;
                    nextInstanceExp++;
                }

                // Set the instance count to that many count and erase the consecutive same meshes
                cpuDrawList[i].instanceCount = nextIdx - i;
                cpuDrawList.erase(cpuDrawList.begin() + i + 1, cpuDrawList.begin() + nextIdx);
            }

            if (cpuDrawList.empty())
            {
                continue;
            }

            // Resize GPU buffer if necessary
            BufferResourceRef bufferRes = shaderMats.drawListPerVertType[drawListIdx];
            if (!bufferRes.isValid() || !bufferRes->isValid() || bufferRes->bufferCount() < cpuDrawList.size())
            {
                bufferRes
                    = graphicsHelper->createReadOnlyIndirectBuffer(graphicsInstance, sizeof(DrawIndexedIndirectCommand), cpuDrawList.size());
                bufferRes->setResourceName(
                    world.getObjectName()
                    + TCHAR("_") + shaderMatsPair.first + EVertexType::toString(EVertexType::Type(vertType)) + TCHAR("_IdxIndirect"));
                bufferRes->init();

                shaderMats.drawListPerVertType[drawListIdx] = bufferRes;
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

void EngineRenderScene::renderTheSceneRenderThread(
    Short2D viewportSize, const Camera &viewCamera, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
    const GraphicsHelperAPI *graphicsHelper
)
{
    IRenderInterfaceModule *renderModule = IRenderInterfaceModule::get();

    LocalPipelineContext pipelineCntxt;
    pipelineCntxt.renderpassFormat = ERenderPassFormat::Generic;
    pipelineCntxt.materialName = TCHAR("ClearRT");
    renderModule->getRenderManager()->preparePipelineContext(&pipelineCntxt, { &lastRT });
    if (!clearParams.isValid())
    {
        clearParams = graphicsHelper->createShaderParameters(graphicsInstance, pipelineCntxt.getPipeline()->getParamLayoutAtSet(0));
        clearParams->setResourceName(TCHAR("ClearParams"));
        clearParams->setVector4Param(STRID("clearColor"), LinearColorConst::CYAN);
        clearParams->init();
    }

    GraphicsPipelineState pipelineState;
    pipelineState.pipelineQuery.drawMode = EPolygonDrawMode::Fill;
    pipelineState.pipelineQuery.cullingMode = ECullingMode::BackFace;

    QuantizedBox2D viewport{
        {             0,              0},
        {viewportSize.x, viewportSize.y}
    };
    QuantizedBox2D scissor{
        {                   viewportSize.x / 4,                    viewportSize.y / 4},
        {viewportSize.x - (viewportSize.x / 4), viewportSize.y - (viewportSize.y / 4)}
    };

    RenderPassAdditionalProps additionalProps;
    additionalProps.bAllowUndefinedLayout = true;
    RenderPassClearValue clearVal;
    clearVal.colors = { LinearColorConst::PALE_BLUE };

    const GraphicsResource *cmdBuffer = cmdList->startCmd(getCmdBufferName(), EQueueFunction::Graphics, true);
    {
        SCOPED_CMD_MARKER(cmdList, cmdBuffer, RenderScene);
        cmdList->cmdBeginRenderPass(cmdBuffer, pipelineCntxt, viewport, additionalProps, clearVal);

        cmdList->cmdBindGraphicsPipeline(cmdBuffer, pipelineCntxt, pipelineState);

        cmdList->cmdBindVertexBuffer(cmdBuffer, 0, GlobalBuffers::getQuadTriVertexBuffer(), 0);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor);
        cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineCntxt, clearParams);

        cmdList->cmdDrawVertices(cmdBuffer, 0, 3);

        cmdList->cmdEndRenderPass(cmdBuffer);
    }
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo2 submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, submitInfo);
}
