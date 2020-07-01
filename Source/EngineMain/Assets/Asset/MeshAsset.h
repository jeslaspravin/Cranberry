#pragma once
#include "AssetObject.h"

class BufferResource;

struct MeshVertexView
{
    uint32 startIndex;
    uint32 numOfIndices;
};

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
};

