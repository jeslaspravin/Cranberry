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

const AABB& MeshAsset::getMeshBounds() const
{
    return bounds;
}

