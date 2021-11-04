#include "MeshAsset.h"

ICleanupAsset* MeshAsset::cleanableAsset()
{
    return this;
}

BufferResource* MeshAsset::getVertexBuffer() const
{
    return vertexBuffer;
}

BufferResource* MeshAsset::getIndexBuffer() const
{
    return indexBuffer;
}

#if _DEBUG
BufferResource* MeshAsset::getTbnVertexBuffer() const
{
    return tbnVertexBuffer;
}
#endif

const AABB& MeshAsset::getMeshBounds() const
{
    return bounds;
}

