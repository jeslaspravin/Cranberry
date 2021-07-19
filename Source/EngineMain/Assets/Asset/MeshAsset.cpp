#include "MeshAsset.h"

ICleanupAsset* MeshAsset::cleanableAsset()
{
    return this;
}

BufferResource* MeshAsset::getVertexBuffer()
{
    return vertexBuffer;
}

BufferResource* MeshAsset::getIndexBuffer()
{
    return indexBuffer;
}

#if _DEBUG
BufferResource* MeshAsset::getTbnVertexBuffer()
{
    return tbnVertexBuffer;
}
#endif

const AABB& MeshAsset::getMeshBounds() const
{
    return bounds;
}

