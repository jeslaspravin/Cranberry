/*!
 * \file EngineRenderScene.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/StringID.h"
#include "Math/Box.h"
#include "Memory/LinearAllocator.h"
#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"
#include "ObjectPtrs.h"
#include "Render/EngineRenderTypes.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderApi/ResourcesInterface/IRenderResource.h"

namespace ERendererIntermTexture
{
enum Type
{
    GBufferDiffuse,
    GBufferNormal,
    GBufferARM,
    TempTest,
    MaxCount
};

EPixelDataFormat::Type getPixelFormat(Type textureType);
const TChar *toString(Type textureType);
} // namespace ERendererIntermTexture

class RendererIntermTexture : public IRenderTargetTexture
{
public:
    ImageResourceRef rtTexture;
    ImageResourceRef resolvedTexture;

    ReferenceCountPtr<MemoryResource> renderResource() const override { return resolvedTexture; }
    ReferenceCountPtr<MemoryResource> renderTargetResource() const override { return rtTexture; };
};

struct TexturePoolListKey
{
    Size3D textureSize;
    // Sample count is not needed as it will most likely will be wait clearing everything on change
    // Layer count is not needed as that will probably same per type of textures in pool

    bool operator==(const TexturePoolListKey &rhs) const { return textureSize == rhs.textureSize; }
};

template <>
struct std::hash<TexturePoolListKey>
{
    NODISCARD size_t operator()(const TexturePoolListKey &val) const noexcept
    {
        return HashUtility::hashAllReturn(val.textureSize.x, val.textureSize.y, val.textureSize.z);
    }
};

class SceneRenderTexturePool
{
private:
    struct TextureData
    {
        RendererIntermTexture intermTexture;
        // If not used after this clear counter reaches 0 this texture will be cleared
        uint32 clearCounter;
    };
    using TextureList = SparseVector<TextureData, BitArraySparsityPolicy>;
    using PoolTexturesMap = std::unordered_multimap<TexturePoolListKey, TextureList::size_type>;
    PoolTexturesMap poolTextures[ERendererIntermTexture::MaxCount];
    TextureList textures;
    uint32 bufferingCount;

public:
    struct PoolTextureDesc
    {
        uint32 layerCount = 1;
        uint32 mipCount = 1;
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1;
    };
    SceneRenderTexturePool(uint32 inBufferingCount)
        : bufferingCount(inBufferingCount)
    {}
    MAKE_TYPE_NONCOPY_NONMOVE(SceneRenderTexturePool)

    // Call getTexture after finishing any command that previously used the texture
    const RendererIntermTexture &
        getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size3D size, PoolTextureDesc textureDesc);
    const RendererIntermTexture &
        getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size2D size, PoolTextureDesc textureDesc);
    // Below function will query existing and return null if nothing found that is use able
    const RendererIntermTexture *getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size3D size);
    const RendererIntermTexture *getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, Size2D size);

    void clearUnused(IRenderCommandList *cmdList);

    // Waits and clears the entire texture pool
    void clearPool(IRenderCommandList *cmdList);
};

// This component is temporary and might change to something better later
struct ComponentRenderInfo
{
    // materialIndex is just a cache. 0 is invalid
    SizeT materialIndex;
    String shaderName;
    StringID materialID;

    // TODO(Jeslas) : Will be changed after other vertex types of meshes are added
    // 0 is invalid
    Transform3D worldTf;
    SizeT tfIndex = 0;

    // vertexBuffers[vertexType].meshes[meshID] gives vertex information for this component
    EVertexType::Type vertexType;
    StringID meshID;

    BufferResourceRef cpuVertBuffer;
    BufferResourceRef cpuIdxBuffer;
};

// 1:1 to each world
class EngineRenderScene
{
private:
    constexpr static const uint32 BUFFER_COUNT = 2;
    constexpr static const uint32 VERTEX_TYPE_COUNT = EVertexType::TypeEnd - EVertexType::TypeStart;

    using RenderInfoVector = SparseVector<ComponentRenderInfo, BitArraySparsityPolicy>;
    static_assert(std::is_same_v<RenderInfoVector::size_type, SizeT>, "Component index type mismatch");

    struct MeshVertexView
    {
        // In counts not bytes
        SizeT vertOffset;
        SizeT vertCount = 0;

        SizeT idxOffset;
        SizeT idxCount = 0;

        SizeT refs = 0;
    };
    struct VerticesPerVertType
    {
        BufferResourceRef vertices;
        BufferResourceRef indices;

        LinearAllocationTracker<1> vertsAllocTracker;
        LinearAllocationTracker<1> idxsAllocTracker;
        std::unordered_map<StringID, MeshVertexView> meshes;

        // List of meshes and Component render info index to add for first time
        std::vector<std::pair<StringID, SizeT>> meshesToAdd;
        // List of meshes to remove in case vertex updates are in progress
        std::vector<StringID> meshesToRemove;

        // Batch copy all vertices
        std::vector<BatchCopyBufferInfo> copies;
    };
    struct InstanceParamsPerVertType
    {
        BufferResourceRef instanceData;
        LinearAllocationTracker<1> allocTracker;

        ShaderParametersRef shaderParameter;

        std::vector<SizeT> compIdxToAdd;
        std::vector<SizeT> instanceIdxToRemove;

        std::vector<BatchCopyBufferInfo> copies;
        std::vector<BatchCopyBufferData> hostToBufferCopies;
    };

    struct MaterialShaderParams
    {
        // Draw lists are for main GBuffer pass. For lights and other rendering passes separate draw list must be maintained
        uint64 drawListCount[VERTEX_TYPE_COUNT];
        BufferResourceRef drawListPerVertType[VERTEX_TYPE_COUNT];

        BufferResourceRef materialData;
        ShaderParametersRef shaderParameter;
        LinearAllocationTracker<1> materialAllocTracker;
        std::vector<uint32> materialRefs;
        // Idx is not material idx but the direct vector idx. No need to do materialIdxToVectorIdx()
        std::unordered_map<StringID, SizeT> materialToIdx;

        std::vector<SizeT> compIdxToAdd;
        std::vector<StringID> materialIDToRemove;

        std::vector<BatchCopyBufferData> drawListCopies;
        std::vector<BatchCopyBufferInfo> materialCopies;
        std::vector<BatchCopyBufferData> hostToMatCopies;
    };

    uint64 frameCount = 0;

    cbe::ObjectPath world;
    RenderInfoVector compsRenderInfo;
    std::unordered_map<StringID, SizeT> componentToRenderInfo;
    ComponentRenderSyncInfo componentUpdates;

    SceneRenderTexturePool rtPool;

    bool bVertexUpdating = false;
    VerticesPerVertType vertexBuffers[VERTEX_TYPE_COUNT];

    bool bMaterialsUpdating = false;
    std::unordered_map<String, MaterialShaderParams> shaderToMaterials;

    bool bInstanceParamsUpdating = false;
    InstanceParamsPerVertType instancesData[VERTEX_TYPE_COUNT];

    String getTransferCmdBufferName() const;
    String getCmdBufferName() const;

    // Initial test code for RT pool
    RendererIntermTexture lastRT;
    const RendererIntermTexture &getTempTexture(IRenderCommandList *cmdList, Short2D size);

    ShaderParametersRef clearParams;

public:
    EngineRenderScene(cbe::World *inWorld);
    MAKE_TYPE_NONCOPY_NONMOVE(EngineRenderScene)

    void clearScene();

    /**
     * Triggers the scene rendering
     */
    void renderTheScene(Short2D viewportSize, const Camera &viewCamera);
    const IRenderTargetTexture *getLastRTResolved() const;
    /**
     * Must be called after RT is copied to back buffer, This returns the textures to pool where it will be waiting till freed
     */
    void onLastRTCopied();

private:
    // Add/Remove mesh ref is used when creating or deleting a component in render scene to clear those vertices in scene vert and index buffers
    FORCE_INLINE void addMeshRef(EVertexType::Type vertType, StringID meshID, SizeT compRenderInfoIdx);
    FORCE_INLINE void removeMeshRef(EVertexType::Type vertType, MeshVertexView &meshVertView, StringID meshID);

    FORCE_INLINE SizeT instanceIdxToVectorIdx(SizeT instanceIdx) const { return instanceIdx - 1; }
    FORCE_INLINE SizeT vectorIdxToInstanceIdx(SizeT idx) const { return idx + 1; }
    FORCE_INLINE void createInstanceCopies(
        InstanceParamsPerVertType &vertInstanceData, const ComponentRenderInfo &compRenderInfo, IRenderCommandList *cmdList,
        IGraphicsInstance *graphicsInstance
    ) const;
    FORCE_INLINE void addCompInstanceData(SizeT compRenderInfoIdx);
    FORCE_INLINE void removeInstanceDataAt(EVertexType::Type vertexType, SizeT instanceIdx);

    FORCE_INLINE SizeT materialIdxToVectorIdx(SizeT materialIdx) const { return materialIdx - 1; }
    FORCE_INLINE SizeT vectorIdxToMaterialIdx(SizeT idx) const { return idx + 1; }
    FORCE_INLINE void createMaterialCopies(
        MaterialShaderParams &shaderMats, const ComponentRenderInfo &compRenderInfo, IRenderCommandList *cmdList,
        IGraphicsInstance *graphicsInstance
    ) const;
    FORCE_INLINE void addCompMaterialData(SizeT compRenderInfoIdx);
    // Why material vector index here alone? Because in materialIDToIdx each material ID directly maps to actual vector idx
    // And only materialIDs to be removed is stored
    FORCE_INLINE void removeMaterialAt(SizeT matVectorIdx, StringID materialID, MaterialShaderParams &shaderMats);

    void addRenderComponents(const std::vector<cbe::RenderableComponent *> &renderComps);
    void removeRenderComponents(const std::vector<StringID> &renderComps);
    void recreateRenderComponents(const std::vector<cbe::RenderableComponent *> &renderComps);
    void updateTfComponents(
        const std::vector<cbe::TransformComponent *> &comps, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance
    );

    void createRenderInfo(cbe::RenderableComponent *comp, SizeT compRenderInfoIdx);
    void destroyRenderInfo(const cbe::RenderableComponent *comp, SizeT compRenderInfoIdx);

    void syncWorldCompsRenderThread(
        const ComponentRenderSyncInfo &compsUpdate, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
        const GraphicsHelperAPI *graphicsHelper
    );
    // Adds new mesh's vertex and index buffer, resizes the corresponding scene buffers to upload them
    copat::NormalFuncAwaiter
        recreateSceneVertexBuffers(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    copat::NormalFuncAwaiter
        recreateMaterialBuffers(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    copat::NormalFuncAwaiter
        recreateInstanceBuffers(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void performTransferCopies(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void renderTheSceneRenderThread(
        Short2D viewportSize, const Camera &viewCamera, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
        const GraphicsHelperAPI *graphicsHelper
    );
};