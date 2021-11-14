#pragma once
#include "Assets/Asset/AssetObject.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "Math/CoreMathTypes.h"
#include "Math/Box.h"
#include "Types/Colors.h"

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
    BufferResourceRef vertexBuffer = nullptr;
    BufferResourceRef indexBuffer = nullptr;

#if _DEBUG
    std::vector<TbnLinePoint> tbnVerts;
    BufferResourceRef tbnVertexBuffer = nullptr;
#endif

    AABB bounds;
public:
    MeshAsset() = default;
    ICleanupAsset* cleanableAsset() override;

    BufferResourceRef getVertexBuffer() const;
    BufferResourceRef getIndexBuffer() const;

#if _DEBUG
    BufferResourceRef getTbnVertexBuffer() const;
#endif

    const AABB& getMeshBounds() const;
};

