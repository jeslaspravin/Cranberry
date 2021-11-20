#include "Assets/Asset/MeshAsset.h"

ICleanupAsset* MeshAsset::cleanableAsset()
{
    return this;
}

BufferResourceRef MeshAsset::getVertexBuffer() const
{
    return vertexBuffer;
}

BufferResourceRef MeshAsset::getIndexBuffer() const
{
    return indexBuffer;
}

#if _DEBUG
BufferResourceRef MeshAsset::getTbnVertexBuffer() const
{
    return tbnVertexBuffer;
}
#endif

const AABB& MeshAsset::getMeshBounds() const
{
    return bounds;
}

