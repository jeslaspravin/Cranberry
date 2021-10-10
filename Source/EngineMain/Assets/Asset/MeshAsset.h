#pragma once
#include "AssetObject.h"
#include "../../Core/Math/CoreMathTypes.h"
#include "../../Core/Math/Box.h"
#include "../../Core/Types/Colors.h"

class BufferResource;

struct MeshVertexView
{
    uint32 startIndex;
    uint32 numOfIndices;
    String name;
};

#if _DEBUG
struct TbnLinePoint
{
    Vector3D position;
    Color color;
};
#endif

class MeshAsset : public AssetBase, public ICleanupAsset
{
/*protected*/public:// TODO: change this back to protected once proper abstraction is added
    BufferResource* vertexBuffer = nullptr;
    BufferResource* indexBuffer = nullptr;

#if _DEBUG
    std::vector<TbnLinePoint> tbnVerts;
    BufferResource* tbnVertexBuffer = nullptr;
#endif

    AABB bounds;
public:
    MeshAsset() = default;
    ICleanupAsset* cleanableAsset() override;

    BufferResource* getVertexBuffer() const;
    BufferResource* getIndexBuffer() const;

#if _DEBUG
    BufferResource* getTbnVertexBuffer() const;
#endif

    const AABB& getMeshBounds() const;
};

