/*!
 * \file ObjStaticMeshImporter.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "StaticMeshImporter.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "CBEPackage.h"
#include "CBEObjectHelpers.h"
#include "Classes/StaticMesh.h"
#include "Classes/World.h"
#include "Math/Math.h"
#include "EditorHelpers.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <array>

template <>
struct std::hash<tinyobj::index_t>
{

    size_t operator()(const tinyobj::index_t keyval) const noexcept
    {
        size_t hashVal = std::hash<int32>{}(keyval.vertex_index);
        HashUtility::hashCombine(hashVal, keyval.normal_index);
        HashUtility::hashCombine(hashVal, keyval.texcoord_index);
        return hashVal;
    }
};
template <>
struct std::equal_to<tinyobj::index_t>
{
    constexpr bool operator()(const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const
    {
        return lhs.vertex_index == rhs.vertex_index && lhs.normal_index == rhs.normal_index && lhs.texcoord_index == rhs.texcoord_index;
    }
};

namespace ObjSMImporterHelpers
{
constexpr inline const uint32 FACE_MAX_VERTS = 3;

enum EImportErrorCodes
{
    DegenerateTextureCoords,
    DegenerateNormals,
    DegenerateTriangle,
    ErrorsCount
};

void printErrors(uint32 errorCount, EImportErrorCodes errorCode)
{
    switch (errorCode)
    {
    case ObjSMImporterHelpers::DegenerateTextureCoords:
        LOG_WARN("ObjStaticMeshImporter", "Incorrect texture coordinate, using world x, y as tangents[%u]", errorCount);
        break;
    case ObjSMImporterHelpers::DegenerateNormals:
        LOG_WARN("ObjStaticMeshImporter", "Degenerate normals, Tangents might be invalid. Expect visual artifacts[%u]", errorCount);
        break;
    case ObjSMImporterHelpers::DegenerateTriangle:
        LOG_WARN("ObjStaticMeshImporter", "Degenerate triangles found and they are removed[%u]", errorCount);
        break;
    case ObjSMImporterHelpers::ErrorsCount:
    default:
        break;
    }
}

struct PerMeshData
{
    std::vector<uint32> indices;
    std::vector<cbe::SMBatchView> meshBatches;
    AABB bound;

    std::vector<cbe::SMTbnLinePoint> tbnVerts;
};
struct IntermediateImportData
{
    StaticMeshImportOptions options;
    std::vector<StaticMeshVertex> vertices;
    std::unordered_map<String, PerMeshData> loadedMeshes;
    // Intermediate values
    std::unordered_map<tinyobj::index_t, uint32> indexToNewVert;
    uint32 errorsCounter[ErrorsCount];
};

bool hasSmoothedNormals(const tinyobj::shape_t &mesh)
{
    for (const uint32 &smoothingGrpIdx : mesh.mesh.smoothing_group_ids)
    {
        if (smoothingGrpIdx > 0)
        {
            return true;
        }
    }
    return false;
}

//
//  Bi-tangent
//  ^
//  |
//  v        v1__________ v2
//  |         /         /
//  |        /         /
//  |     v0/_________/
//  |
//  +------------ u --> Tangent
//
//  v0 to v1 (v1 - v0) = (u1 - u0) * T + (v1 - v0) * B
//  Solve the same for other pair v0, v2
//
void calcTangent(
    ArrayView<uint32> errorCountersView, PerMeshData &loaderData, StaticMeshVertex &vertexData, const StaticMeshVertex &other1,
    const StaticMeshVertex &other2
)
{
    const Vector2D uv10{ other1.position.w() - vertexData.position.w(), other1.normal.w() - vertexData.normal.w() };
    const Vector2D uv20{ other2.position.w() - vertexData.position.w(), other2.normal.w() - vertexData.normal.w() };

    const Vector3D p10 = Vector3D(other1.position) - Vector3D(vertexData.position);
    const Vector3D p20 = Vector3D(other2.position) - Vector3D(vertexData.position);

    const Vector3D normal{ vertexData.normal };
    Vector3D tangent;
    Vector3D bitangent;

    float invDet = uv10.x() * uv20.y() - uv20.x() * uv10.y();
    if (invDet == 0.0f)
    {
        Rotation tbnFrame = RotationMatrix::fromZ(normal).asRotation();
        tangent = tbnFrame.fwdVector();
        bitangent = tbnFrame.rightVector();

        errorCountersView[EImportErrorCodes::DegenerateTextureCoords]++;
    }
    else
    {
        invDet = 1 / invDet;

        tangent = invDet * (uv20.y() * p10 - uv10.y() * p20);
        // Gram-Schmidt orthogonalize
        tangent = tangent.rejectFrom(normal).normalized();

        bitangent = invDet * (uv10.x() * p20 - uv20.x() * p10);
        // Gram-Schmidt orthogonalize
        bitangent = bitangent.rejectFrom(normal).rejectFrom(tangent).normalized();

        /**
         * Handedness - dot(cross(normal(z) ^ tangent(x)), bitangent) must be positive
         */
        if (((normal ^ tangent) | bitangent) < 0)
        {
            tangent = -tangent;
        }
    }

    // vertexData.bitangent = Vector4D(bitangent, 0);
    vertexData.tangent = Vector4D(tangent, 0);

    const float drawLen = 10;
    cbe::SMTbnLinePoint pt1;
    pt1.position = Vector3D(vertexData.position);
    cbe::SMTbnLinePoint pt2;

    // Normal
    pt2.position = pt1.position + (normal * drawLen); // 10 cm
    pt1.color = ColorConst::BLUE;
    pt2.color = ColorConst::BLUE;
    loaderData.tbnVerts.emplace_back(pt1);
    loaderData.tbnVerts.emplace_back(pt2);

    // Tangent
    pt2.position = pt1.position + (tangent * drawLen); // 10 cm
    pt1.color = ColorConst::RED;
    pt2.color = ColorConst::RED;
    loaderData.tbnVerts.emplace_back(pt1);
    loaderData.tbnVerts.emplace_back(pt2);

    // Bi-Tangent
    pt2.position = pt1.position + (bitangent * drawLen); // 10 cm
    pt1.color = ColorConst::GREEN;
    pt2.color = ColorConst::GREEN;
    loaderData.tbnVerts.emplace_back(pt1);
    loaderData.tbnVerts.emplace_back(pt2);
}

void fillVertexInfo(StaticMeshVertex &vertexData, const tinyobj::attrib_t &attrib, const tinyobj::index_t &index)
{
    // Inverting Y since UV origin is at left bottom of image and Graphics API's UV origin is at left top
    Vector2D uvCoord{ attrib.texcoords[index.texcoord_index * 2 + 0], (1.0f - attrib.texcoords[index.texcoord_index * 2 + 1]) };
    uvCoord = Math::clamp(uvCoord, Vector2D::ZERO, Vector2D::ONE);

    vertexData.position = Vector4D(
        attrib.vertices[index.vertex_index * 3], attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2],
        uvCoord.x()
    );
    Vector3D normal{ attrib.normals[index.normal_index * 3], attrib.normals[index.normal_index * 3 + 1],
                     attrib.normals[index.normal_index * 3 + 2] };

    vertexData.normal = Vector4D(normal.safeNormalized(), uvCoord.y());
    // vertexData.vertexColor = Vector4D(attrib.colors[index.vertex_index * 3], attrib.colors[index.vertex_index * 3 + 1],
    // attrib.colors[index.vertex_index * 2 + 2], 1.0f);
}

bool isDegenerateTri(uint32 index0, uint32 index1, uint32 index2, const std::vector<StaticMeshVertex> &verticesData)
{
    Vector3D dir1 = Vector3D(verticesData[index1].position) - Vector3D(verticesData[index0].position);
    Vector3D dir2 = Vector3D(verticesData[index2].position) - Vector3D(verticesData[index0].position);

    return (dir1 ^ dir2).sqrlength() < SLIGHTLY_SMALL_EPSILON;
}
Vector3D getFaceNormal(uint32 index0, uint32 index1, uint32 index2, const std::vector<StaticMeshVertex> &verticesData)
{
    debugAssert(!isDegenerateTri(index0, index1, index2, verticesData));

    Vector3D dir1 = Vector3D(verticesData[index1].position) - Vector3D(verticesData[index0].position);
    Vector3D dir2 = Vector3D(verticesData[index2].position) - Vector3D(verticesData[index0].position);

    return (dir1 ^ dir2).normalized();
}

void addNormal(StaticMeshVertex &vertex, Vector3D &normal)
{
    Vector4D &encodedNormal = vertex.normal;
    encodedNormal.x() += normal.x();
    encodedNormal.y() += normal.y();
    encodedNormal.z() += normal.z();
}
void normalize(Vector4D &normal)
{
    Vector3D newNormal(normal);
    newNormal = newNormal.normalized();
    normal.x() = newNormal.x();
    normal.y() = newNormal.y();
    normal.z() = newNormal.z();
}

void splitMeshBatches(
    PerMeshData &meshImportData, const std::vector<int32> &faceMaterialId, const std::vector<tinyobj::material_t> &materials,
    uint32 uniqueMatCount, uint32 faceCount
)
{
    // Splitting based on face material IDs
    if (uniqueMatCount > 1)
    {
        std::unordered_map<uint32, std::vector<uint32>> materialIdToIndices;

        for (uint32 faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            std::vector<uint32> &indices = materialIdToIndices[faceMaterialId[faceIdx]];

            uint32 faceStartIndex = faceIdx * FACE_MAX_VERTS;
            for (uint32 i = 0; i < 3; ++i)
            {
                indices.push_back(meshImportData.indices[faceStartIndex + i]);
            }
        }

        meshImportData.indices.clear();
        meshImportData.indices.reserve(faceCount * FACE_MAX_VERTS);
        meshImportData.meshBatches.clear();
        meshImportData.meshBatches.reserve(materialIdToIndices.size());
        for (const std::pair<const uint32, std::vector<uint32>> &matIdIndices : materialIdToIndices)
        {
            cbe::SMBatchView vertexBatchView;
            vertexBatchView.startIndex = uint32(meshImportData.indices.size());
            vertexBatchView.numOfIndices = uint32(matIdIndices.second.size());
            vertexBatchView.name = UTF8_TO_TCHAR(materials[matIdIndices.first].name.c_str());
            vertexBatchView.name.trim();
            if (vertexBatchView.name.empty())
            {
                vertexBatchView.name = TCHAR("MeshBatch_") + String::toString(meshImportData.meshBatches.size());
            }
            meshImportData.indices.insert(meshImportData.indices.end(), matIdIndices.second.cbegin(), matIdIndices.second.cend());
            meshImportData.meshBatches.push_back(vertexBatchView);
        }
    }
    else
    {
        cbe::SMBatchView vertexBatchView;
        vertexBatchView.startIndex = 0;
        vertexBatchView.numOfIndices = uint32(meshImportData.indices.size());
        meshImportData.meshBatches.push_back(vertexBatchView);
    }
}

void load(
    IntermediateImportData &outImportData, const tinyobj::shape_t &mesh, const tinyobj::attrib_t &attrib,
    const std::vector<tinyobj::material_t> &materials
)
{
    PerMeshData &meshImportData = outImportData.loadedMeshes[UTF8_TO_TCHAR(mesh.name.c_str())];
    meshImportData.indices.resize(mesh.mesh.indices.size());

    uint32 faceCount = uint32(mesh.mesh.indices.size() / 3);

    std::vector<int32> faceMaterialId(faceCount);
    std::set<int32> uniqueMatIds;
    // Vertices pushed to meshImportData along with indices
    {
        std::unordered_map<tinyobj::index_t, uint32> &indexToNewVert = outImportData.indexToNewVert;
        for (uint32 faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            debugAssert(FACE_MAX_VERTS == 3 && FACE_MAX_VERTS == mesh.mesh.num_face_vertices[faceIdx]);

            const std::array<tinyobj::index_t, FACE_MAX_VERTS> idxs
                = { mesh.mesh.indices[faceIdx * FACE_MAX_VERTS + 0], mesh.mesh.indices[faceIdx * FACE_MAX_VERTS + 1],
                    mesh.mesh.indices[faceIdx * FACE_MAX_VERTS + 2] };

            std::array<uint32, FACE_MAX_VERTS> newVertIdxs;

            faceMaterialId[faceIdx] = mesh.mesh.material_ids[faceIdx];
            uniqueMatIds.insert(mesh.mesh.material_ids[faceIdx]);

            // Filling vertex data to mesh struct
            std::array<bool, FACE_MAX_VERTS> bIsNewlyAdded;
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                auto newVertIdxItr = indexToNewVert.find(idxs[i]);
                if (newVertIdxItr == indexToNewVert.end())
                {
                    uint32 &vertexIdx = indexToNewVert[idxs[i]];
                    vertexIdx = uint32(outImportData.vertices.size());

                    newVertIdxs[i] = vertexIdx;
                    outImportData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(outImportData.vertices[vertexIdx], attrib, idxs[i]);
                    bIsNewlyAdded[i] = true;
                }
                else
                {
                    newVertIdxs[i] = newVertIdxItr->second;
                    bIsNewlyAdded[i] = false;
                }
            }

            bool bIsDegenTri = isDegenerateTri(newVertIdxs[0], newVertIdxs[1], newVertIdxs[2], outImportData.vertices);
            // If degenerate then remove inserted vertices as correcting normals/calculating Tangents might fail
            if (bIsDegenTri)
            {
                for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
                {
                    if (bIsNewlyAdded[i])
                    {
                        indexToNewVert.erase(idxs[i]);
                        if (outImportData.vertices.size() > newVertIdxs[i])
                        {
                            // remove added vertices at end of vector
                            outImportData.vertices.resize(newVertIdxs[i]);
                        }
                    }
                }
                outImportData.errorsCounter[EImportErrorCodes::DegenerateTriangle]++;
                continue;
            }

            // Fixing triangle and vertex discrepancies
            Vector3D faceNormal = getFaceNormal(newVertIdxs[0], newVertIdxs[1], newVertIdxs[2], outImportData.vertices);
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                if (bIsNewlyAdded[i])
                {
                    meshImportData.bound.grow(Vector3D(outImportData.vertices[newVertIdxs[i]].position));
                    // Invalid normal, use faceNormal. It will not be invalid as degenerate case is handled already
                    if (outImportData.vertices[newVertIdxs[i]].normal.sqrlength() < SLIGHTLY_SMALL_EPSILON)
                    {
                        outImportData.vertices[newVertIdxs[i]].normal = Vector4D(faceNormal, outImportData.vertices[newVertIdxs[i]].normal.w());
                        outImportData.errorsCounter[EImportErrorCodes::DegenerateNormals]++;
                    }
                }
            }
            // Since we need all three vertex for tangent and bi-tangent calculations
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                if (bIsNewlyAdded[i])
                {
                    calcTangent(
                        outImportData.errorsCounter, meshImportData, outImportData.vertices[newVertIdxs[i]],
                        outImportData.vertices[newVertIdxs[(i + 1) % FACE_MAX_VERTS]],
                        outImportData.vertices[newVertIdxs[(i + 2) % FACE_MAX_VERTS]]
                    );
                }
            }
            // makeCCW(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                meshImportData.indices[faceIdx * FACE_MAX_VERTS + i] = newVertIdxs[i];
            }
        }
    }

    splitMeshBatches(meshImportData, faceMaterialId, materials, uint32(uniqueMatIds.size()), faceCount);
}

void smoothAndLoad(
    IntermediateImportData &outImportData, const tinyobj::shape_t &mesh, const tinyobj::attrib_t &attrib,
    const std::vector<tinyobj::material_t> &materials
)
{
    const float smoothingThreshold = Math::cos(Math::deg2Rad(outImportData.options.smoothingAngle));
    PerMeshData &meshImportData = outImportData.loadedMeshes[UTF8_TO_TCHAR(mesh.name.c_str())];
    meshImportData.indices.resize(mesh.mesh.indices.size());

    uint32 faceCount = uint32(mesh.mesh.indices.size() / 3);

    std::vector<int32> faceMaterialId(faceCount);
    std::set<int32> uniqueMatIds;
    // Vertices pushed to meshImportData along with indices
    {
        std::unordered_map<tinyobj::index_t, uint32> &indexToNewVert = outImportData.indexToNewVert;
        // maps an edge formed per vertex and all connected vertices to all faces sharing the edge
        std::unordered_map<uint32, std::unordered_map<uint32, std::vector<uint32>>> vertexFaceAdjacency;
        std::vector<Vector3D> faceNormals(faceCount);
        std::vector<uint32> faceSmoothingId(faceCount);

        for (uint32 faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            debugAssert(FACE_MAX_VERTS == 3 && FACE_MAX_VERTS == mesh.mesh.num_face_vertices[faceIdx]);

            const std::array<tinyobj::index_t, FACE_MAX_VERTS> idxs
                = { mesh.mesh.indices[faceIdx * FACE_MAX_VERTS + 0], mesh.mesh.indices[faceIdx * FACE_MAX_VERTS + 1],
                    mesh.mesh.indices[faceIdx * FACE_MAX_VERTS + 2] };

            std::array<uint32, FACE_MAX_VERTS> newVertIdxs;

            faceSmoothingId[faceIdx] = mesh.mesh.smoothing_group_ids[faceIdx];
            faceMaterialId[faceIdx] = mesh.mesh.material_ids[faceIdx];
            uniqueMatIds.insert(mesh.mesh.material_ids[faceIdx]);

            // Filling vertex data to mesh struct
            std::array<bool, FACE_MAX_VERTS> bIsNewlyAdded;
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                auto newVertIdxItr = indexToNewVert.find(idxs[i]);
                if (newVertIdxItr == indexToNewVert.end())
                {
                    uint32 &vertexIdx = indexToNewVert[idxs[i]];
                    vertexIdx = uint32(outImportData.vertices.size());

                    newVertIdxs[i] = vertexIdx;
                    outImportData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(outImportData.vertices[vertexIdx], attrib, idxs[i]);
                    bIsNewlyAdded[i] = true;
                }
                else
                {
                    newVertIdxs[i] = newVertIdxItr->second;
                    bIsNewlyAdded[i] = false;
                }
            }

            bool bIsDegenTri = isDegenerateTri(newVertIdxs[0], newVertIdxs[1], newVertIdxs[2], outImportData.vertices);
            // If degenerate then remove inserted vertices as correcting normals/calculating Tangents might fail
            if (bIsDegenTri)
            {
                for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
                {
                    if (bIsNewlyAdded[i])
                    {
                        indexToNewVert.erase(idxs[i]);
                        if (outImportData.vertices.size() > newVertIdxs[i])
                        {
                            // remove added vertices at end of vector
                            outImportData.vertices.resize(newVertIdxs[i]);
                        }
                    }
                }
                outImportData.errorsCounter[EImportErrorCodes::DegenerateTriangle]++;
                continue;
            }

            // Fixing triangle and vertex discrepancies
            Vector3D faceNormal = getFaceNormal(newVertIdxs[0], newVertIdxs[1], newVertIdxs[2], outImportData.vertices);
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                if (bIsNewlyAdded[i])
                {
                    meshImportData.bound.grow(Vector3D(outImportData.vertices[newVertIdxs[i]].position));
                    // Invalid normal, use faceNormal. It will not be invalid as degenerate case is handled already
                    if (outImportData.vertices[newVertIdxs[i]].normal.sqrlength() < SLIGHTLY_SMALL_EPSILON)
                    {
                        outImportData.vertices[newVertIdxs[i]].normal = Vector4D(faceNormal, outImportData.vertices[newVertIdxs[i]].normal.w());
                        outImportData.errorsCounter[EImportErrorCodes::DegenerateNormals]++;
                    }
                }
            }
            // Since we need all three vertex for tangent and bi-tangent calculations
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                if (bIsNewlyAdded[i])
                {
                    calcTangent(
                        outImportData.errorsCounter, meshImportData, outImportData.vertices[newVertIdxs[i]],
                        outImportData.vertices[newVertIdxs[(i + 1) % FACE_MAX_VERTS]],
                        outImportData.vertices[newVertIdxs[(i + 2) % FACE_MAX_VERTS]]
                    );
                }
            }
            // makeCCW(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                meshImportData.indices[faceIdx * FACE_MAX_VERTS + i] = newVertIdxs[i];
            }

            // Prepare smoothing data
            faceNormals[faceIdx] = faceNormal;
            // Fill vertex pair's(Edge's) faces adjacency
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                for (uint32 j = i; i != FACE_MAX_VERTS; ++i)
                {

                    uint32 vertIdx0 = newVertIdxs[i], vertIdx1 = newVertIdxs[j];
                    // Find if we already have edge to faces maps for one of two vertices
                    auto vertFaceAdjItr = vertexFaceAdjacency.find(newVertIdxs[i]);
                    if (vertFaceAdjItr == vertexFaceAdjacency.end())
                    {
                        std::swap(vertIdx0, vertIdx1);
                        vertFaceAdjItr = vertexFaceAdjacency.find(newVertIdxs[j]);
                    }
                    // both vertices are not present in map yet
                    if (vertFaceAdjItr == vertexFaceAdjacency.end())
                    {
                        // Swap again to keep consistency (i, j)
                        std::swap(vertIdx0, vertIdx1);
                    }
                    vertexFaceAdjacency[vertIdx0][vertIdx1].emplace_back(faceIdx);
                }
            }
        }

        uint32 originalVertCount = uint32(outImportData.vertices.size());
        for (uint32 vertIdx = 0; vertIdx < originalVertCount; ++vertIdx)
        {
            std::vector<std::set<uint32>> faceGroups;
            auto vertFaceAdjItr = vertexFaceAdjacency.find(vertIdx);
            if (vertFaceAdjItr == vertexFaceAdjacency.cend())
            {
                continue;
            }

            auto collectSmoothingFaceGrps
                = [&faceGroups, &smoothingThreshold](float dotVal, bool bIsSameSmoothing, const std::array<uint32, 2> &adjFaceIdxs)
            {
                debugAssert(adjFaceIdxs[0] != adjFaceIdxs[1]);

                if (dotVal >= smoothingThreshold && bIsSameSmoothing) // Is same face group
                {
                    // Find each of smoothing face groups that at least one face index exists
                    uint32 grpsFoundCount = 0;
                    std::array<uint32, 2> faceGrpsFound;
                    for (uint32 faceGrpIdx = 0; faceGrpIdx != faceGroups.size(); ++faceGrpIdx)
                    {
                        if (faceGroups[faceGrpIdx].contains(adjFaceIdxs[0]) || faceGroups[faceGrpIdx].contains(adjFaceIdxs[1]))
                        {
                            faceGrpsFound[grpsFoundCount] = faceGrpIdx;
                            grpsFoundCount++;
                        }
                    }
                    debugAssert(grpsFoundCount <= 2);

                    if (grpsFoundCount == 0)
                    {
                        faceGroups.push_back({ adjFaceIdxs[0], adjFaceIdxs[1] });
                    }
                    else if (grpsFoundCount == 1)
                    {
                        faceGroups[faceGrpsFound[0]].insert(adjFaceIdxs[0]);
                        faceGroups[faceGrpsFound[0]].insert(adjFaceIdxs[1]);
                    }
                    else
                    {
                        // Merge second face groups into one single smoothed group
                        faceGroups[faceGrpsFound[0]].insert(faceGroups[faceGrpsFound[1]].begin(), faceGroups[faceGrpsFound[1]].end());
                        faceGroups.erase(faceGroups.begin() + faceGrpsFound[1]);
                    }
                }
                else // Non smoothing case
                {
                    for (uint32 faceIdx : adjFaceIdxs)
                    {
                        bool bIsInserted = false;
                        for (uint32 faceGrpIdx = 0; faceGrpIdx < faceGroups.size(); ++faceGrpIdx)
                        {
                            auto faceItr = faceGroups[faceGrpIdx].find(faceIdx);
                            if (faceItr != faceGroups[faceGrpIdx].end())
                            {
                                bIsInserted = true;
                            }
                        }
                        if (!bIsInserted)
                        {
                            faceGroups.push_back({ faceIdx });
                        }
                    }
                }
            };
            for (const std::pair<const uint32, std::vector<uint32>> &adjacentFaces : vertFaceAdjItr->second)
            {
                float dotVal = 1;
                bool bIsSameSmoothing = true;
                if (adjacentFaces.second.size() == 2)
                {
                    float dotVal = faceNormals[adjacentFaces.second[0]] | faceNormals[adjacentFaces.second[1]];
                    bool bIsSameSmoothing = faceSmoothingId[adjacentFaces.second[0]] == faceSmoothingId[adjacentFaces.second[1]];
                    std::array<uint32, 2> adjFaceIdxs{ adjacentFaces.second[0], adjacentFaces.second[1] };
                    collectSmoothingFaceGrps(dotVal, bIsSameSmoothing, adjFaceIdxs);
                }
                else
                {
                    // if more than 2 then we have to smooth every combination
                    for (uint32 i = 0; i != adjacentFaces.second.size(); ++i)
                    {
                        for (uint32 j = i + 1; j != adjacentFaces.second.size(); ++j)
                        {
                            float dotVal = faceNormals[adjacentFaces.second[i]] | faceNormals[adjacentFaces.second[j]];
                            bool bIsSameSmoothing = faceSmoothingId[adjacentFaces.second[j]] == faceSmoothingId[adjacentFaces.second[i]];
                            std::array<uint32, 2> adjFaceIdxs{ adjacentFaces.second[i], adjacentFaces.second[j] };
                            collectSmoothingFaceGrps(dotVal, bIsSameSmoothing, adjFaceIdxs);
                        }
                    }
                }
            }

            // for each face groups from 1 to end, copy the vertex corresponding to vertIdx to new vertex and generate smoothed normal
            for (auto faceGrpsItr = faceGroups.begin() + 1; faceGrpsItr != faceGroups.end(); ++faceGrpsItr)
            {
                uint32 newVertIndex = uint32(outImportData.vertices.size());
                outImportData.vertices.push_back(outImportData.vertices[vertIdx]);

                for (const uint32 &faceIdx : *faceGrpsItr)
                {
                    uint32 faceStartIndex = faceIdx * FACE_MAX_VERTS;
                    for (uint32 i = 0; i < FACE_MAX_VERTS; ++i)
                    {
                        if (vertIdx == meshImportData.indices[faceStartIndex + i])
                        {
                            meshImportData.indices[faceStartIndex + i] = newVertIndex;
                            addNormal(outImportData.vertices[newVertIndex], faceNormals[faceIdx]);
                            break;
                        }
                    }
                }
            }
            // Smooth vertIdx vertex as well as this vertex is most likely will be unique to this mesh
            for (const uint32 &faceIdx : faceGroups[0])
            {
                uint32 faceStartIndex = faceIdx * FACE_MAX_VERTS;
                for (uint32 i = 0; i < FACE_MAX_VERTS; ++i)
                {
                    if (vertIdx == meshImportData.indices[faceStartIndex + i])
                    {
                        addNormal(outImportData.vertices[vertIdx], faceNormals[faceIdx]);
                        break;
                    }
                }
            }
        }
    }

    splitMeshBatches(meshImportData, faceMaterialId, materials, uint32(uniqueMatIds.size()), faceCount);
}

} // namespace ObjSMImporterHelpers

#undef TINYOBJLOADER_IMPLEMENTATION

bool ObjStaticMeshImporter::supportsImporting(ImportOption &inOutOptions)
{
    if (inOutOptions.fileExt.isEqual(TCHAR("OBJ"), false))
    {
        inOutOptions.optionsStruct = &options;
        inOutOptions.structType = StaticMeshImportOptions::staticType();

        return true;
    }
    return false;
}

std::vector<cbe::Object *> ObjStaticMeshImporter::tryImporting(const ImportOption &importOptions) const
{
    std::vector<cbe::Object *> importedObjs;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> meshes;
    std::vector<tinyobj::material_t> materials;

    std::string warning;
    std::string error;
    bool bIsSuccessful = tinyobj::LoadObj(
        &attrib, &meshes, &materials, &warning, &error, TCHAR_TO_ANSI(importOptions.filePath.getChar()),
        TCHAR_TO_ANSI(importOptions.fileDirectory.getChar())
    );
    if (!warning.empty())
    {
        LOG_WARN("ObjStaticMeshImporter", "Tiny obj loader %s", UTF8_TO_TCHAR(warning.c_str()));
    }
    if (!error.empty())
    {
        LOG_ERROR("ObjStaticMeshImporter", "Tiny obj loader %s", UTF8_TO_TCHAR(error.c_str()));
        return importedObjs;
    }
    if (!bIsSuccessful)
    {
        LOG_ERROR("ObjStaticMeshImporter", "Loading %s with ObjStaticMeshImporter failed!", importOptions.filePath);
        return importedObjs;
    }
    if (meshes.empty())
    {
        LOG_WARN("ObjStaticMeshImporter", "No mesh found while loading %s with ObjStaticMeshImporter!", importOptions.filePath);
        return importedObjs;
    }

    ObjSMImporterHelpers::IntermediateImportData meshIntermediate;
    meshIntermediate.options = options;
    CBEMemory::memZero(&meshIntermediate.errorsCounter, sizeof(meshIntermediate.errorsCounter));
    for (tinyobj::shape_t &mesh : meshes)
    {
        const bool hasSmoothing = ObjSMImporterHelpers::hasSmoothedNormals(mesh);

        if (options.bLoadSmoothed && !hasSmoothing)
        {
            ObjSMImporterHelpers::smoothAndLoad(meshIntermediate, mesh, attrib, materials);
        }
        else
        {
            ObjSMImporterHelpers::load(meshIntermediate, mesh, attrib, materials);
        }

        // It is okay to check this every loop, Not profiled
        if (!(options.bImportAllMesh || options.bImportAsScene))
        {
            break;
        }
    }
    // Normalizing all the vertex normals
    for (StaticMeshVertex &vertex : meshIntermediate.vertices)
    {
        ObjSMImporterHelpers::normalize(vertex.normal);
    }
    // Print errors
    bool bHadAnyErrors = false;
    for (uint32 i = 0; i != ObjSMImporterHelpers::ErrorsCount; ++i)
    {
        if (meshIntermediate.errorsCounter[i] > 0)
        {
            bHadAnyErrors = true;
            break;
        }
    }
    if (bHadAnyErrors)
    {
        LOG_WARN("ObjStaticMeshImporter", "Errors when loading mesh %s", importOptions.filePath);
        for (uint32 i = 0; i != ObjSMImporterHelpers::ErrorsCount; ++i)
        {
            if (meshIntermediate.errorsCounter[i] > 0)
            {
                ObjSMImporterHelpers::printErrors(meshIntermediate.errorsCounter[i], ObjSMImporterHelpers::EImportErrorCodes(i));
            }
        }
    }

    // Split each mesh and it's vertices
    std::unordered_map<String, cbe::SMCreateInfo> createInfoSMs;
    createInfoSMs.reserve(meshIntermediate.loadedMeshes.size());
    for (std::pair<const String, ObjSMImporterHelpers::PerMeshData> &meshIntermData : meshIntermediate.loadedMeshes)
    {
        cbe::SMCreateInfo &createInfo = createInfoSMs[PropertyHelper::getValidSymbolName(meshIntermData.first)];
        createInfo.meshBatches = std::move(meshIntermData.second.meshBatches);
        createInfo.bounds = std::move(meshIntermData.second.bound);
        createInfo.tbnVerts = std::move(meshIntermData.second.tbnVerts);

        // Assign indices and vertices per mesh after converting intermediate vertices in to per mesh vertices
        createInfo.indices.reserve(meshIntermData.second.indices.size());
        std::unordered_map<uint32, uint32> vertIntermIdxToMeshIdx;
        for (uint32 vertIdx : meshIntermData.second.indices)
        {
            auto meshVertIdxItr = vertIntermIdxToMeshIdx.find(vertIdx);
            if (meshVertIdxItr == vertIntermIdxToMeshIdx.cend())
            {
                const uint32 index = uint32(createInfo.vertices.size());
                vertIntermIdxToMeshIdx[vertIdx] = index;
                createInfo.indices.emplace_back(index);
                createInfo.vertices.emplace_back(meshIntermediate.vertices[vertIdx]);
            }
            else
            {
                createInfo.indices.emplace_back(meshVertIdxItr->second);
            }
        }
    }

    auto createStaticMesh
        = [this, &importOptions](String &packageName, const String &meshName, cbe::SMCreateInfo &&createInfo) -> cbe::StaticMesh *
    {
        makePackageUnique(packageName);

        cbe::Package *package = cbe::Package::createPackage(packageName, importOptions.importContentPath, false);
        debugAssert(package);
        cbe::markDirty(package);
        SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);

        cbe::StaticMesh *mesh = cbe::create<cbe::StaticMesh, cbe::SMCreateInfo &&>(
            meshName, package, cbe::EObjectFlagBits::ObjFlag_PackageLoaded, std::forward<decltype(createInfo)>(createInfo)
        );
        return mesh;
    };

    if (options.bImportAsScene || options.bImportAllMesh)
    {
        String packageRelDir = importOptions.relativeDirPath + ObjectPathHelper::ObjectObjectSeparator + importOptions.fileName;
        for (std::pair<const String, cbe::SMCreateInfo> &smCI : createInfoSMs)
        {
            String packageName = packageRelDir + ObjectPathHelper::ObjectObjectSeparator + smCI.first;
            importedObjs.emplace_back(createStaticMesh(packageName, smCI.first, std::move(smCI.second)));
        }
    }
    else
    {
        debugAssert(createInfoSMs.size() == 1);
        String packageName = importOptions.relativeDirPath + ObjectPathHelper::ObjectObjectSeparator + importOptions.fileName;

        importedObjs.emplace_back(createStaticMesh(packageName, importOptions.fileName, std::move(createInfoSMs.begin()->second)));
    }

    if (options.bImportAsScene && !importedObjs.empty())
    {
        String packageName = importOptions.relativeDirPath + ObjectPathHelper::ObjectObjectSeparator + importOptions.fileName;
        cbe::Package *worldPackage = cbe::Package::createPackage(packageName, importOptions.importContentPath, false);
        debugAssert(worldPackage);
        cbe::markDirty(worldPackage);
        SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(worldPackage), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);

        std::vector<cbe::StaticMesh *> staticMeshes;
        staticMeshes.resize(importedObjs.size());
        CBEMemory::memCopy(staticMeshes.data(), importedObjs.data(), sizeof(cbe::StaticMesh *) * importedObjs.size());

        cbe::World *world = cbe::create<cbe::World>(importOptions.fileName, worldPackage, cbe::EObjectFlagBits::ObjFlag_PackageLoaded);
        cbe::Actor *rootActor = EditorHelpers::addStaticMeshesToWorld(staticMeshes, world, importOptions.fileName + TCHAR("Root"));
        debugAssert(world && rootActor);

        importedObjs.insert(importedObjs.begin(), world);
    }
    return importedObjs;
}

void ObjStaticMeshImporter::makePackageUnique(String &inOutPackageDir) const
{
    String outPath = inOutPackageDir;
    uint32 counter = 0;
    while (cbe::get(outPath.getChar()))
    {
        counter++;
        outPath = inOutPackageDir + String::toString(counter);
    }
    inOutPackageDir = outPath;
}