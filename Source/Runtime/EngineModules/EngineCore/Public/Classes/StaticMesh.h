/*!
 * \file StaticMesh.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "Math/Box.h"
#include "CBEObject.h"
#include "RenderApi/VertexData.h"
#include "RenderInterface/Resources/MemoryResources.h"

#include "StaticMesh.gen.h"

class IRenderCommandList;
class IGraphicsInstance;
class GraphicsHelperAPI;

namespace cbe
{
struct SMBatchView
{
    uint32 startIndex;
    uint32 numOfIndices;
    String name;
};

struct SMTbnLinePoint
{
    Vector3 position;
    Color color;
};

struct META_ANNOTATE() SMCreateInfo
{
    GENERATED_CODES()

    std::vector<StaticMeshVertex> vertices;
    std::vector<uint32> indices;
    std::vector<SMBatchView> meshBatches;
    AABB bounds;

    std::vector<SMTbnLinePoint> tbnVerts;
};

class ENGINECORE_EXPORT StaticMesh : public Object
{
    GENERATED_CODES()
public:
#if EDITOR_BUILD
    // Editor only to allow changing vertices and indices easily
    std::vector<StaticMeshVertex> vertices;
    std::vector<uint32> indices;
#endif
    std::vector<SMBatchView> meshBatches;
    AABB bounds;

    std::vector<SMTbnLinePoint> tbnVerts;
#if EDITOR_BUILD
    BufferResourceRef tbnVertexBuffer = nullptr;
#endif

    BufferResourceRef vertexCpuBuffer;
    BufferResourceRef indexCpuBuffer;
    // Following will be valid only after corresponding CPU buffers are created in render thread
    ArrayRange<StaticMeshVertex> vertexCpuView;
    ArrayRange<uint32> indexCpuView;

    StaticMesh();
    StaticMesh(SMCreateInfo &&ci);

    /* cbe::Object overrides */
    void destroy() override;
    ObjectArchive &serialize(ObjectArchive &ar) override;
    /* Overrides ends */

private:
    void copyResources(
        const std::vector<StaticMeshVertex> &inVertices, const std::vector<uint32> &inIndices, IRenderCommandList *cmdList,
        IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
    );

} META_ANNOTATE(NoExport);

} // namespace cbe