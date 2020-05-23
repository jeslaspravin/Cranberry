#pragma once
#include "AssetObject.h"
#include "../../RenderInterface/ShaderCore/ShaderParameters.h"

class BufferResource;

class MeshAsset : public AssetBase, public ICleanupAsset
{
/*protected*/public:// TODO: change this back to protected once proper abstraction is added
    BufferResource* vertexBuffer = nullptr;
    BufferResource* indexBuffer = nullptr;
public:
    MeshAsset() = default;
    ICleanupAsset* cleanableAsset() override;

    BufferResource* getVertexBuffer();
    BufferResource* getIndexBuffer();

    template<typename MeshType>
    static ShaderVertexParamInfo* getShaderParamInfo();
};

template<>
ShaderVertexParamInfo* MeshAsset::getShaderParamInfo<class StaticMeshAsset>();

template<typename MeshType>
ShaderVertexParamInfo* MeshAsset::getShaderParamInfo()
{
    return nullptr;
}

