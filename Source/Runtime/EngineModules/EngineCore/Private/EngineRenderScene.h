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
#include "ObjectPtrs.h"
#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"
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
};

// 1:1 to each world
class EngineRenderScene
{
private:
    constexpr static const uint32 BUFFER_COUNT = 2;
    struct ComponentRenderInfo
    {
        EVertexType::Type vertexType;
        SizeT tfIndex;
        SizeT materialIndex;
    };

    cbe::ObjectPath world;
    SparseVector<ComponentRenderInfo, BitArraySparsityPolicy> compsRenderInfo;
    std::unordered_map<StringID, SizeT> componentToRenderInfo;

    SceneRenderTexturePool rtPool;

    // Initial test code for RT pool
    uint64 frameCount = 0;
    RendererIntermTexture lastRT;
    const RendererIntermTexture &getTempTexture(IRenderCommandList *cmdList, Short2D size);
    String getCmdBufferName() const;

    ShaderParametersRef clearParams;

public:
    EngineRenderScene(cbe::World *inWorld);
    MAKE_TYPE_NONCOPY_NONMOVE(EngineRenderScene)

    copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread> syncWorldComps(ComponentRenderSyncInfo compsUpdate);

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
    void createRenderInfo(cbe::RenderableComponent *comp, ComponentRenderInfo &outRenderInfo) const;
    void destroyRenderInfo(const ComponentRenderInfo &renderInfo) const;

    void renderTheSceneRenderThread(
        Short2D viewportSize, const Camera &viewCamera, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
        const GraphicsHelperAPI *graphicsHelper
    );
};