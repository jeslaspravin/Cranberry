#pragma once
#include "AssetObject.h"
#include "../../Core/Math/CoreMathTypes.h"
#include "../../Core/Math/Box.h"

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

    AABB bounds;
public:
    MeshAsset() = default;
    ICleanupAsset* cleanableAsset() override;

    BufferResource* getVertexBuffer();
    BufferResource* getIndexBuffer();

    const AABB& getMeshBounds() const;
};

