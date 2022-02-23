/*!
 * \file MeshAsset.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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

#if DEV_BUILD
BufferResourceRef MeshAsset::getTbnVertexBuffer() const
{
    return tbnVertexBuffer;
}
#endif

const AABB& MeshAsset::getMeshBounds() const
{
    return bounds;
}

