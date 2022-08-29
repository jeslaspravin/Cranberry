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
} // namespace ERendererIntermTexture

class RendererIntermTexture : public IRenderTargetTexture
{
public:
    ImageResourceRef rtTexture;
    ImageResourceRef resolvedTexture;

    ReferenceCountPtr<MemoryResource> renderResource() const override { return resolvedTexture; }
    ReferenceCountPtr<MemoryResource> renderTargetResource() const override { return rtTexture; };
};

struct RendererIntermTextureList
{
    RendererIntermTexture gbufferDiffuse;
    RendererIntermTexture gbufferNormal;
    RendererIntermTexture gbufferARM;
    RendererIntermTexture tempTestTexture;

    RendererIntermTexture *textures[ERendererIntermTexture::MaxCount];
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

    // Initial test code for RT pool
    uint64 frameCount = 0;
    RendererIntermTexture lastRT;
    std::vector<RendererIntermTexture> tempTextures;
    std::vector<RendererIntermTexture> texturesToClear;
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