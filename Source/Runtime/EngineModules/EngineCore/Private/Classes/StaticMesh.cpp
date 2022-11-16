/*!
 * \file StaticMesh.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/StaticMesh.h"
#include "Serialization/CommonTypesSerialization.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, cbe::SMBatchView &value)
{
    return archive << value.startIndex << value.numOfIndices << value.name;
}
template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, cbe::SMTbnLinePoint &value)
{
    return archive << value.position << value.color;
}
template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, StaticMeshVertex &value)
{
    return archive << value.position << value.normal << value.tangent;
}

namespace cbe
{
constexpr inline const uint32 STATIC_MESH_SERIALIZER_VERSION = 0;
constexpr inline const uint32 STATIC_MESH_SERIALIZER_CUTOFF_VERSION = 0;
STRINGID_CONSTEXPR inline const StringID STATIC_MESH_CUSTOM_VERSION_ID = STRID("StaticMeshSerializer");

StaticMesh::StaticMesh() {}

StaticMesh::StaticMesh(SMCreateInfo &&ci)
{
    meshBatches = std::move(ci.meshBatches);
    bounds = std::move(ci.bounds);
#if EDITOR_BUILD
    vertices = std::move(ci.vertices);
    indices = std::move(ci.indices);
    tbnVerts = std::move(ci.tbnVerts);

    ENQUEUE_RENDER_COMMAND(CreateStateMesh)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            copyResources(vertices, indices, cmdList, graphicsInstance, graphicsHelper);

            // Copy tangent, binormal, normal vertices
            tbnVertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(SMTbnLinePoint), uint32(tbnVerts.size()));
            tbnVertexBuffer->setResourceName(getName() + TCHAR("_TbnVerts"));
            tbnVertexBuffer->init();
            cmdList->copyToBuffer(tbnVertexBuffer, 0, tbnVerts.data(), uint32(tbnVertexBuffer->getResourceSize()));
        }
    );
#else
    ENQUEUE_RENDER_COMMAND(CreateStateMesh)
    (
        [this, ci = std::forward(ci)](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            copyResources(ci.vertices, ci.indices, cmdList, graphicsInstance, graphicsHelper);
        }
    );
#endif
}

void StaticMesh::destroy()
{
#if EDITOR_BUILD
    vertices.clear();
    indices.clear();
#endif
    meshBatches.clear();
    bounds = {};

    ENQUEUE_RENDER_COMMAND(DestroyStaticMesh)
    (
        [vertsBuffer = vertexCpuBuffer,
         idxBuffer = indexCpuBuffer](IRenderCommandList *, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper) mutable
        {
            graphicsHelper->returnMappedPtr(graphicsInstance, vertsBuffer);
            graphicsHelper->returnMappedPtr(graphicsInstance, idxBuffer);
        }
    );
    vertexCpuBuffer.reset();
    indexCpuBuffer.reset();
    vertexCpuView.reset();
    indexCpuView.reset();
}

ObjectArchive &StaticMesh::serialize(ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 dataVersion = ar.getCustomVersion(uint32(STATIC_MESH_CUSTOM_VERSION_ID));
        // This must crash
        fatalAssertf(
            STATIC_MESH_SERIALIZER_CUTOFF_VERSION >= dataVersion,
            "Version of Static mesh %u loaded from package %s is outdated, Minimum supported %u!", dataVersion, getOuterMost()->getFullPath(),
            STATIC_MESH_SERIALIZER_CUTOFF_VERSION
        );
    }
    else
    {
        ar.setCustomVersion(uint32(STATIC_MESH_CUSTOM_VERSION_ID), STATIC_MESH_SERIALIZER_VERSION);
    }

    // Once I have ways to cook separate for runtime I can move tbnVerts as Editor only
    ar << tbnVerts;
#if EDITOR_BUILD
    if (ar.isLoading())
    {
        ENQUEUE_RENDER_COMMAND(LoadTBNData)
        (
            [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                // Copy tangent, binormal, normal vertices
                tbnVertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(SMTbnLinePoint), uint32(tbnVerts.size()));
                tbnVertexBuffer->setResourceName(getName() + TCHAR("_TbnVerts"));
                tbnVertexBuffer->init();
                cmdList->copyToBuffer(tbnVertexBuffer, 0, tbnVerts.data(), uint32(tbnVertexBuffer->getResourceSize()));
            }
        );
    }

    // Serializing actual data
    ar << vertices;
    ar << indices;
#else
    std::vector<StaticMeshVertex> vertices;
    std::vector<uint32> indices;
    if (ar.isLoading())
    {
        ar << vertices;
        ar << indices;
    }
    else
    {
        fatalAssert(indexCpuView.ptr() != nullptr && vertexCpuView.ptr() != nullptr);
        // Serialize in same way std::vector will be serialized
        SizeT len = vertexCpuView.size();
        ar << len;
        for (StaticMeshVertex &vert : vertexCpuView)
        {
            ar << vert;
        }

        SizeT len = indexCpuView.size();
        ar << len;
        for (uint32 &idx : indexCpuView)
        {
            ar << idx;
        }
    }
#endif
    ar << meshBatches;
    ar << bounds;
    if (ar.isLoading())
    {
        ENQUEUE_RENDER_COMMAND(LoadStaticMesh)
        (
            [this, inVertices = vertices,
             inIndices = indices](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                copyResources(inVertices, inIndices, cmdList, graphicsInstance, graphicsHelper);
            }
        );
    }
    return ar;
}

void StaticMesh::copyResources(
    const std::vector<StaticMeshVertex> &inVertices, const std::vector<uint32> &inIndices, IRenderCommandList *,
    IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    vertexCpuBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, uint32(sizeof(StaticMeshVertex)), uint32(inVertices.size()));
    vertexCpuBuffer->setAsStagingResource(true);
    vertexCpuBuffer->setResourceName(getName() + TCHAR("_CPUVerts"));
    vertexCpuBuffer->init();

    indexCpuBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, uint32(sizeof(uint32)), uint32(inIndices.size()));
    indexCpuBuffer->setAsStagingResource(true);
    indexCpuBuffer->setResourceName(getName() + TCHAR("_CPUIndices"));
    indexCpuBuffer->init();

    vertexCpuView = { static_cast<StaticMeshVertex *>(graphicsHelper->borrowMappedPtr(graphicsInstance, vertexCpuBuffer)),
                      vertexCpuBuffer->bufferCount() };
    indexCpuView = { static_cast<uint32 *>(graphicsHelper->borrowMappedPtr(graphicsInstance, indexCpuBuffer)), indexCpuBuffer->bufferCount() };

    CBEMemory::memCopy(vertexCpuView.ptr(), inVertices.data(), vertexCpuBuffer->getResourceSize());
    CBEMemory::memCopy(indexCpuView.ptr(), inIndices.data(), indexCpuBuffer->getResourceSize());

    graphicsHelper->flushMappedPtr(graphicsInstance, std::vector<BufferResourceRef>{ vertexCpuBuffer, indexCpuBuffer });
}

} // namespace cbe
