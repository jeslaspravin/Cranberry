/*!
 * \file MeshAsset.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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

#if DEV_BUILD
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

#if DEV_BUILD
    std::vector<TbnLinePoint> tbnVerts;
    BufferResourceRef tbnVertexBuffer = nullptr;
#endif

    AABB bounds;
public:
    MeshAsset() = default;
    ICleanupAsset* cleanableAsset() override;

    BufferResourceRef getVertexBuffer() const;
    BufferResourceRef getIndexBuffer() const;

#if DEV_BUILD
    BufferResourceRef getTbnVertexBuffer() const;
#endif

    const AABB& getMeshBounds() const;
};

