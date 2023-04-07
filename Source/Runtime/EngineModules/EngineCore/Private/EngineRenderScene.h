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

#include "Math/Box.h"
#include "Memory/FreeListAllocator.h"
#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "Types/Camera/Camera.h"
#include "ObjectPtrs.h"
#include "Render/EngineRenderTypes.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Resources/BufferedResources.h"

#include <bitset>

struct DrawIndexedIndirectCommand;
namespace copat
{
struct NormalFuncAwaiter;
}

namespace ERendererIntermTexture
{
enum Type
{
    GBufferDiffuse,
    GBufferNormal,
    GBufferARM,
    GBufferDepth,
    FinalColor,
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
    ReferenceCountPtr<MemoryResource> renderTargetResource() const override { return rtTexture; }
};

struct TexturePoolListKey
{
    UInt3 textureSize;
    // Sample count is not needed as it will most likely will be wait clearing everything on change
    // Layer count is not needed as that will probably same per type of textures in pool

    bool operator== (const TexturePoolListKey &rhs) const { return textureSize == rhs.textureSize; }
};

template <>
struct std::hash<TexturePoolListKey>
{
    NODISCARD size_t operator() (const TexturePoolListKey &val) const noexcept
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
    getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt3 size, PoolTextureDesc textureDesc);
    const RendererIntermTexture &
    getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt2 size, PoolTextureDesc textureDesc);
    // Below function will query existing and return null if nothing found that is use able
    const RendererIntermTexture *getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt3 size);
    const RendererIntermTexture *getTexture(IRenderCommandList *cmdList, ERendererIntermTexture::Type rtType, UInt2 size);

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
    cbe::ObjectPath matObjPath;

    // TODO(Jeslas) : Will be changed after other vertex types of meshes are added
    // 0 is invalid
    SizeT tfIndex = 0;
    Transform3D worldTf;
    AABB worldBound;

    // vertexBuffers[vertexType].meshes[meshID] gives vertex information for this component
    EVertexType::Type vertexType;
    cbe::ObjectPath meshObjPath;

    BufferResourceRef cpuVertBuffer;
    BufferResourceRef cpuIdxBuffer;

    // Will be same as one mapped in componentToRenderInfo
    cbe::ObjectPath compObjPath;
};

struct RenderSceneViewParams
{
    Camera view;
    Short2 viewportSize;

    ERendererIntermTexture::Type outBuffer = ERendererIntermTexture::FinalColor;
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

        FreeListAllocTracker<1> vertsAllocTracker;
        FreeListAllocTracker<1> idxsAllocTracker;
        std::unordered_map<cbe::ObjectPath, MeshVertexView> meshes;

        // List of meshes and Component render info index to add for first time
        std::vector<std::pair<cbe::ObjectPath, SizeT>> meshesToAdd;
        // List of meshes to remove in case vertex updates are in progress
        std::vector<cbe::ObjectPath> meshesToRemove;

        // Batch copy all vertices
        std::vector<BatchCopyBufferInfo> copies;
    };
    struct InstanceParamsPerVertType
    {
        BufferResourceRef instanceData;
        FreeListAllocTracker<1> allocTracker;

        ShaderParametersRef shaderParameter;

        std::vector<SizeT> compIdxToAdd;
        std::vector<SizeT> instanceIdxToRemove;

        std::vector<BatchCopyBufferInfo> copies;
        std::vector<BatchCopyBufferData> hostToBufferCopies;
    };

    struct MaterialShaderParams
    {
        constexpr static const uint32 DRAWLIST_BUFFERED_COUNT = VERTEX_TYPE_COUNT * BUFFER_COUNT;
        // Draw lists are for main GBuffer pass. For lights and other rendering passes separate draw list must be maintained
        BufferResourceRef drawListPerVertType[DRAWLIST_BUFFERED_COUNT];
        uint32 drawListCounts[DRAWLIST_BUFFERED_COUNT] = {};
        // If the draw counts data buffer is at least copied once, This is to avoid unnecessary barrier before first copy
        std::bitset<DRAWLIST_BUFFERED_COUNT> drawListCopied;
        std::vector<DrawIndexedIndirectCommand> cpuDrawListPerVertType[VERTEX_TYPE_COUNT];

        BufferResourceRef materialData;
        ShaderParametersRef shaderParameter;
        FreeListAllocTracker<1> materialAllocTracker;
        std::vector<uint32> materialRefs;
        // Idx is not material idx but the direct vector idx. No need to do materialIdxToVectorIdx()
        std::unordered_map<cbe::ObjectPath, SizeT> materialToIdx;

        std::vector<SizeT> compIdxToAdd;
        std::vector<cbe::ObjectPath> materialIDToRemove;

        std::vector<BatchCopyBufferData> drawListCopies;
        std::vector<BatchCopyBufferInfo> materialCopies;
        std::vector<BatchCopyBufferData> hostToMatCopies;
        bool bMatsCopied = false;
    };

    uint64 frameCount = 0;

    cbe::ObjectPath world;
    RenderInfoVector compsRenderInfo;
    BitArray<uint64> compsVisibility;
    std::unordered_map<cbe::ObjectPath, SizeT> componentToRenderInfo;
    ComponentRenderSyncInfo componentUpdates;

    SceneRenderTexturePool rtPool;

    bool bVertexUpdating = false;
    VerticesPerVertType vertexBuffers[VERTEX_TYPE_COUNT];
    // If the vertex and index buffer is at least copied once, This is to avoid unnecessary barrier before first copy
    std::bitset<VERTEX_TYPE_COUNT> vertIdxBufferCopied;

    bool bMaterialsUpdating = false;
    std::unordered_map<String, MaterialShaderParams> shaderToMaterials;

    bool bInstanceParamsUpdating = false;
    InstanceParamsPerVertType instancesData[VERTEX_TYPE_COUNT];
    // If the instances data buffer is at least copied once, This is to avoid unnecessary barrier before first copy
    std::bitset<VERTEX_TYPE_COUNT> instanceDataCopied;

    // Scene common data
    RingBufferedResource<ShaderParametersRef, BUFFER_COUNT> bindlessSet;
    RingBufferedResource<ShaderParametersRef, BUFFER_COUNT> sceneViewParams;

    String getTransferCmdBufferName() const;
    String getCmdBufferName() const;

    RingBufferedResource<ShaderParametersRef, BUFFER_COUNT> colorResolveParams;
    RingBufferedResource<ShaderParametersRef, BUFFER_COUNT> depthResolveParams;
    RendererIntermTexture frameTextures[ERendererIntermTexture::MaxCount];
    const RendererIntermTexture &getFinalColor(IRenderCommandList *cmdList, Short2 size);

public:
    EngineRenderScene(cbe::World *inWorld);
    MAKE_TYPE_NONCOPY_NONMOVE(EngineRenderScene)
    ~EngineRenderScene();

    void clearScene();

    /**
     * Triggers the scene rendering
     */
    void renderTheScene(RenderSceneViewParams viewParams);
    const IRenderTargetTexture *getLastRTResolved() const;
    /**
     * Must be called after RT is copied to back buffer, This returns the textures to pool where it will be waiting till freed
     */
    void onLastRTCopied();

private:
    // Add/Remove mesh ref is used when creating or deleting a component in render scene to clear those vertices in scene vert and index buffers
    FORCE_INLINE void addMeshRef(EVertexType::Type vertType, cbe::ObjectPath meshFullPath, SizeT compRenderInfoIdx);
    FORCE_INLINE void removeMeshRef(EVertexType::Type vertType, MeshVertexView &meshVertView, cbe::ObjectPath meshFullPath);

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
        MaterialShaderParams &shaderMats, SizeT materialIdx, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance
    ) const;
    FORCE_INLINE void addCompMaterialData(SizeT compRenderInfoIdx);
    // Why material vector index here alone? Because in materialIDToIdx each material ID directly maps to actual vector idx
    // And only materialIDs to be removed is stored
    FORCE_INLINE void removeMaterialAt(SizeT matVectorIdx, cbe::ObjectPath matFullPath, MaterialShaderParams &shaderMats);

    FORCE_INLINE uint32 getBufferedReadOffset() const { return uint32(frameCount % BUFFER_COUNT); }
    FORCE_INLINE uint32 getBufferedWriteOffset() const { return uint32((frameCount + 1) % BUFFER_COUNT); }

    void addRenderComponents(const std::vector<cbe::RenderableComponent *> &renderComps);
    void removeRenderComponents(const std::vector<String> &renderComps);
    void recreateRenderComponents(const std::vector<cbe::RenderableComponent *> &renderComps);
    void
    updateTfComponents(const std::vector<cbe::TransformComponent *> &comps, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance);

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
    void createNextDrawList(
        const RenderSceneViewParams &viewParams, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
        const GraphicsHelperAPI *graphicsHelper
    );
    void performTransferCopies(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);

    void updateVisibility(const RenderSceneViewParams &viewParams);

    void initRenderThread(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void renderTheSceneRenderThread(
        const RenderSceneViewParams &viewParams, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
        const GraphicsHelperAPI *graphicsHelper
    );
};