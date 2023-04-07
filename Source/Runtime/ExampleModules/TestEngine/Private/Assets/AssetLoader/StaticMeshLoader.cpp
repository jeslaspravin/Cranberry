/*!
 * \file StaticMeshLoader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/AssetLoader/StaticMeshLoader.h"
#include "Assets/Asset/StaticMeshAsset.h"
#include "Assets/AssetLoaderLibrary.h"
#include "Logger/Logger.h"
#include "Math/Box.h"
#include "Math/Math.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "String/String.h"
#include "Types/Platform/LFS/PlatformLFS.h"

#include <algorithm>
#include <array>
#include <set>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct MeshLoaderData
{
    std::vector<StaticMeshVertex> vertices;
    std::vector<uint32> indices;
    std::vector<MeshVertexView> meshBatches;
    AABB bound;

#if DEV_BUILD
    std::vector<TbnLinePoint> tbnVerts;
#endif
};

template <>
struct std::hash<tinyobj::index_t>
{

    size_t operator() (const tinyobj::index_t keyval) const noexcept
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
    constexpr bool operator() (const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const
    {
        return lhs.vertex_index == rhs.vertex_index && lhs.normal_index == rhs.normal_index && lhs.texcoord_index == rhs.texcoord_index;
    }
};

bool hasSmoothedNormals(const tinyobj::shape_t &mesh)
{
    for (uint32 smoothingGrpIdx : mesh.mesh.smoothing_group_ids)
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
//   ------------ u --> Tangent
//
//  v0 to v1 (v1 - v0) = (u1 - u0) * T + (v1 - v0) * B
//  Solve the same for other pair v0, v2
//
void calcTangent(MeshLoaderData &loaderData, StaticMeshVertex &vertexData, const StaticMeshVertex &other1, const StaticMeshVertex &other2)
{
    const Vector2 uv10{ other1.position.w() - vertexData.position.w(), other1.normal.w() - vertexData.normal.w() };
    const Vector2 uv20{ other2.position.w() - vertexData.position.w(), other2.normal.w() - vertexData.normal.w() };

    const Vector3 p10 = Vector3(other1.position) - Vector3(vertexData.position);
    const Vector3 p20 = Vector3(other2.position) - Vector3(vertexData.position);

    const Vector3 normal{ vertexData.normal };
    Vector3 tangent;
    Vector3 bitangent;

    float invDet = uv10.x() * uv20.y() - uv20.x() * uv10.y();
    if (invDet == 0.0f)
    {
        LOG_DEBUG("StaticMeshLoader", "Incorrect texture coordinate, using world x, y as tangents");

        Rotation tbnFrame = RotationMatrix::fromZ(normal).asRotation();
        tangent = tbnFrame.fwdVector();
        bitangent = tbnFrame.rightVector();
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

        //
        // Handedness - dot(cross(normal(z) ^ tangent(x)), bitangent) must be positive
        //
        if (((normal ^ tangent) | bitangent) < 0)
        {
            tangent = -tangent;
        }
    }

    // vertexData.bitangent = Vector4(bitangent, 0);
    vertexData.tangent = Vector4(tangent, 0);

#if DEV_BUILD
    const float drawLen = 10;
    TbnLinePoint pt1;
    pt1.position = Vector3(vertexData.position);
    TbnLinePoint pt2;

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
#else
    CompilerHacks::ignoreUnused(loaderData);
#endif
}

void fillVertexInfo(StaticMeshVertex &vertexData, const tinyobj::attrib_t &attrib, const tinyobj::index_t &index)
{
    // Inverting Y since UV origin is at left bottom of image and Graphics API's UV origin is at left top
    Vector2 uvCoord{ attrib.texcoords[index.texcoord_index * 2 + 0], (1.0f - attrib.texcoords[index.texcoord_index * 2 + 1]) };
    uvCoord = Math::clamp(uvCoord, Vector2::ZERO, Vector2::ONE);

    vertexData.position = Vector4(
        attrib.vertices[index.vertex_index * 3], attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2],
        uvCoord.x()
    );
    Vector3 normal{ attrib.normals[index.normal_index * 3], attrib.normals[index.normal_index * 3 + 1],
                    attrib.normals[index.normal_index * 3 + 2] };

    vertexData.normal = Vector4(normal.normalized(), uvCoord.y());
    // vertexData.vertexColor = Vector4(attrib.colors[index.vertex_index * 3],
    // attrib.colors[index.vertex_index * 3 + 1]
    //     , attrib.colors[index.vertex_index * 3 + 2], 1.0f);
}

Vector3 StaticMeshLoader::getFaceNormal(uint32 index0, uint32 index1, uint32 index2, const std::vector<StaticMeshVertex> &verticesData) const
{
    Vector4 temp1 = verticesData[index1].position - verticesData[index0].position;
    Vector4 temp2 = verticesData[index2].position - verticesData[index0].position;

    Vector3 dir1(temp1.x(), temp1.y(), temp1.z());
    Vector3 dir2(temp2.x(), temp2.y(), temp2.z());

    return (dir1 ^ dir2).normalized();
}

void StaticMeshLoader::addNormal(StaticMeshVertex &vertex, Vector3 &normal) const
{
    Vector4 &encodedNormal = vertex.normal;
    encodedNormal.x() += normal.x();
    encodedNormal.y() += normal.y();
    encodedNormal.z() += normal.z();
}

void StaticMeshLoader::normalize(Vector4 &normal) const
{
    Vector3 newNormal(normal);
    newNormal = newNormal.normalized();
    normal.x() = newNormal.x();
    normal.y() = newNormal.y();
    normal.z() = newNormal.z();
}

void StaticMeshLoader::load(const tinyobj::shape_t &mesh, const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials)
{
    MeshLoaderData &meshLoaderData = loadedMeshes[UTF8_TO_TCHAR(mesh.name.c_str())];
    meshLoaderData.indices.resize(mesh.mesh.indices.size());

    uint32 faceCount = uint32(mesh.mesh.indices.size() / 3);

    std::vector<int32> faceMaterialId(faceCount);
    std::set<int32> uniqueMatIds;
    // Vertices pushed to meshLoaderData along with indices
    {
        std::unordered_map<tinyobj::index_t, uint32> indexToNewVert;
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
            std::array<bool, FACE_MAX_VERTS> generateTbn;
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                auto newVertIdxItr = indexToNewVert.find(idxs[i]);
                if (newVertIdxItr == indexToNewVert.end())
                {
                    uint32 &vertexIdx = indexToNewVert[idxs[i]];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdxs[i] = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idxs[i]);
                    meshLoaderData.bound.grow(Vector3(meshLoaderData.vertices[vertexIdx].position));
                    generateTbn[i] = true;
                }
                else
                {
                    newVertIdxs[i] = newVertIdxItr->second;
                    generateTbn[i] = false;
                }
            }

            // Since we need all three vertex for tangent and bi-tangent calculations
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                if (generateTbn[i])
                {
                    calcTangent(
                        meshLoaderData, meshLoaderData.vertices[newVertIdxs[i]], meshLoaderData.vertices[newVertIdxs[(i + 1) % FACE_MAX_VERTS]],
                        meshLoaderData.vertices[newVertIdxs[(i + 2) % FACE_MAX_VERTS]]
                    );
                }
            }
            // makeCCW(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                meshLoaderData.indices[faceIdx * FACE_MAX_VERTS + i] = newVertIdxs[i];
            }
        }
    }

    splitMeshBatches(meshLoaderData, faceMaterialId, materials, uint32(uniqueMatIds.size()), faceCount);

    // Normalizing all the vertex normals
    for (StaticMeshVertex &vertex : meshLoaderData.vertices)
    {
        normalize(vertex.normal);
    }
}

void StaticMeshLoader::smoothAndLoad(
    const tinyobj::shape_t &mesh, const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials
)
{
    const float smoothingThreshold = Math::cos(Math::deg2Rad(smoothingAngle));
    MeshLoaderData &meshLoaderData = loadedMeshes[UTF8_TO_TCHAR(mesh.name.c_str())];
    meshLoaderData.indices.resize(mesh.mesh.indices.size());

    uint32 faceCount = uint32(mesh.mesh.indices.size() / 3);

    std::vector<int32> faceMaterialId(faceCount);
    std::set<int32> uniqueMatIds;
    // Vertices pushed to meshLoaderData along with indices
    {
        std::unordered_map<tinyobj::index_t, uint32> indexToNewVert;
        // maps an edge formed per vertex and all connected vertices to all faces sharing the edge
        std::unordered_map<uint32, std::unordered_map<uint32, std::vector<uint32>>> vertexFaceAdjacency;
        std::vector<Vector3> faceNormals(faceCount);
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
            std::array<bool, FACE_MAX_VERTS> generateTbn;
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                auto newVertIdxItr = indexToNewVert.find(idxs[i]);
                if (newVertIdxItr == indexToNewVert.end())
                {
                    uint32 &vertexIdx = indexToNewVert[idxs[i]];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdxs[i] = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idxs[i]);
                    meshLoaderData.bound.grow(Vector3(meshLoaderData.vertices[vertexIdx].position));
                    generateTbn[i] = true;
                }
                else
                {
                    newVertIdxs[i] = newVertIdxItr->second;
                    generateTbn[i] = false;
                }
            }

            // Since we need all three vertex for tangent and bi-tangent calculations
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                if (generateTbn[i])
                {
                    calcTangent(
                        meshLoaderData, meshLoaderData.vertices[newVertIdxs[i]], meshLoaderData.vertices[newVertIdxs[(i + 1) % FACE_MAX_VERTS]],
                        meshLoaderData.vertices[newVertIdxs[(i + 2) % FACE_MAX_VERTS]]
                    );
                }
            }
            // makeCCW(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);
            for (uint32 i = 0; i != FACE_MAX_VERTS; ++i)
            {
                meshLoaderData.indices[faceIdx * FACE_MAX_VERTS + i] = newVertIdxs[i];
            }

            // Prepare smoothing data
            faceNormals[faceIdx] = getFaceNormal(newVertIdxs[0], newVertIdxs[1], newVertIdxs[2], meshLoaderData.vertices);
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

        uint32 originalVertCount = uint32(meshLoaderData.vertices.size());
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
                uint32 newVertIndex = uint32(meshLoaderData.vertices.size());
                meshLoaderData.vertices.push_back(meshLoaderData.vertices[vertIdx]);

                for (uint32 faceIdx : *faceGrpsItr)
                {
                    uint32 faceStartIndex = faceIdx * FACE_MAX_VERTS;
                    for (uint32 i = 0; i < FACE_MAX_VERTS; ++i)
                    {
                        if (vertIdx == meshLoaderData.indices[faceStartIndex + i])
                        {
                            meshLoaderData.indices[faceStartIndex + i] = newVertIndex;
                            addNormal(meshLoaderData.vertices[newVertIndex], faceNormals[faceIdx]);
                            break;
                        }
                    }
                }
            }
            // Smooth vertIdx vertex as well as this vertex is most likely will be unique to this mesh
            for (uint32 faceIdx : faceGroups[0])
            {
                uint32 faceStartIndex = faceIdx * FACE_MAX_VERTS;
                for (uint32 i = 0; i < FACE_MAX_VERTS; ++i)
                {
                    if (vertIdx == meshLoaderData.indices[faceStartIndex + i])
                    {
                        addNormal(meshLoaderData.vertices[vertIdx], faceNormals[faceIdx]);
                        break;
                    }
                }
            }
        }
    }

    splitMeshBatches(meshLoaderData, faceMaterialId, materials, uint32(uniqueMatIds.size()), faceCount);

    // Normalizing all the vertex normals
    for (StaticMeshVertex &vertex : meshLoaderData.vertices)
    {
        normalize(vertex.normal);
    }
}

void StaticMeshLoader::splitMeshBatches(
    MeshLoaderData &meshLoaderData, const std::vector<int32> &faceMaterialId, const std::vector<tinyobj::material_t> &materials,
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

            uint32 faceStartIndex = faceIdx * 3;
            for (uint32 i = 0; i < 3; ++i)
            {
                indices.push_back(meshLoaderData.indices[faceStartIndex + i]);
            }
        }

        meshLoaderData.indices.clear();
        meshLoaderData.indices.reserve(faceCount * 3);
        meshLoaderData.meshBatches.clear();
        meshLoaderData.meshBatches.reserve(materialIdToIndices.size());
        for (const std::pair<const uint32, std::vector<uint32>> &matIdIndices : materialIdToIndices)
        {
            MeshVertexView vertexBatchView;
            vertexBatchView.startIndex = uint32(meshLoaderData.indices.size());
            vertexBatchView.numOfIndices = uint32(matIdIndices.second.size());
            vertexBatchView.name = String(UTF8_TO_TCHAR(materials[matIdIndices.first].name.c_str())).trimCopy();
            meshLoaderData.indices.insert(meshLoaderData.indices.end(), matIdIndices.second.cbegin(), matIdIndices.second.cend());
            meshLoaderData.meshBatches.push_back(vertexBatchView);
        }
    }
    else
    {
        MeshVertexView vertexBatchView;
        vertexBatchView.startIndex = 0;
        vertexBatchView.numOfIndices = uint32(meshLoaderData.indices.size());
        meshLoaderData.meshBatches.push_back(vertexBatchView);
    }
}

StaticMeshLoader::StaticMeshLoader(const String &assetPath)
    : bIsSuccessful(false)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> meshes;
    std::vector<tinyobj::material_t> materials;

    std::string warning;
    std::string error;
    bIsSuccessful = tinyobj::LoadObj(
        &attrib, &meshes, &materials, &warning, &error, TCHAR_TO_ANSI(assetPath.getChar()),
        TCHAR_TO_ANSI(PlatformFile(assetPath).getHostDirectory().getChar())
    );
    if (!warning.empty())
    {
        LOG_WARN("StaticMeshLoader", "Tiny obj loader %s", UTF8_TO_TCHAR(warning.c_str()));
    }
    if (!error.empty())
    {
        LOG_ERROR("StaticMeshLoader", "Tiny obj loader %s", UTF8_TO_TCHAR(error.c_str()));
        return;
    }

    for (tinyobj::shape_t &mesh : meshes)
    {
        const bool hasSmoothing = hasSmoothedNormals(mesh);

        if (bLoadSmoothed && !hasSmoothing)
        {
            smoothAndLoad(mesh, attrib, materials);
        }
        else
        {
            load(mesh, attrib, materials);
        }
    }
}
#undef TINYOBJLOADER_IMPLEMENTATION

bool StaticMeshLoader::fillAssetInformation(const std::vector<StaticMeshAsset *> &assets) const
{
    if (bIsSuccessful)
    {
        uint32 idx = 0;
        for (const std::pair<const String, MeshLoaderData> &meshDataPair : loadedMeshes)
        {
            StaticMeshAsset *staticMesh = assets[idx];
            staticMesh->setAssetName(meshDataPair.first);
            staticMesh->vertices = meshDataPair.second.vertices;
            staticMesh->indices = meshDataPair.second.indices;
            staticMesh->meshBatches = meshDataPair.second.meshBatches;
            staticMesh->bounds = meshDataPair.second.bound;

#if DEV_BUILD
            staticMesh->tbnVerts = meshDataPair.second.tbnVerts;
#endif
            ++idx;
        }
    }
    return bIsSuccessful;
}

uint32 StaticMeshLoader::getMeshNum() const { return uint32(loadedMeshes.size()); }

void AssetLoaderLibrary::loadStaticMesh(const String &assetPath, std::vector<AssetBase *> &staticMeshes)
{
    StaticMeshLoader loader(assetPath);
    std::vector<StaticMeshAsset *> meshes(loader.getMeshNum());

    for (StaticMeshAsset *&mesh : meshes)
    {
        mesh = new StaticMeshAsset();
    }

    if (loader.fillAssetInformation(meshes))
    {
        staticMeshes.resize(meshes.size());
        for (uint32 i = 0; i < meshes.size(); ++i)
        {
            staticMeshes[i] = meshes[i];
        }
        return;
    }
    for (StaticMeshAsset *&mesh : meshes)
    {
        delete mesh;
    }
    meshes.clear();
    staticMeshes.clear();
}
