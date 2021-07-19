#include "StaticMeshLoader.h"
#include "../../Core/String/String.h"
#include "../../Core/Logger/Logger.h"
#include "../AssetLoaderLibrary.h"
#include "../Asset/StaticMeshAsset.h"
#include "../../Core/Math/Vector3D.h"
#include "../../Core/Math/Vector2D.h"
#include "../../Core/Math/Math.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"
#include "../../Core/Math/Box.h"

#include <unordered_map>
#include <set>
#include <array>
#include <algorithm>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct MeshLoaderData
{
    std::vector<StaticMeshVertex> vertices;
    std::vector<uint32> indices;
    std::vector<MeshVertexView> meshBatches;
    AABB bound{ Vector3D::ZERO, Vector3D::ZERO };

#if _DEBUG
    std::vector<TbnLinePoint> tbnVerts;
#endif
};

template <>
struct std::hash<tinyobj::index_t> {

    size_t operator()(const tinyobj::index_t keyval) const noexcept {
        size_t hashVal = std::hash<int32>{}(keyval.vertex_index);
        HashUtility::hashCombine(hashVal, std::hash<int32>{}(keyval.texcoord_index));
        HashUtility::hashCombine(hashVal, std::hash<int32>{}(keyval.normal_index));
        return hashVal;
    }
};
template <>
struct std::equal_to<tinyobj::index_t> {
    constexpr bool operator()(const tinyobj::index_t& lhs, const tinyobj::index_t& rhs) const {
        return lhs.vertex_index == rhs.vertex_index && lhs.normal_index == rhs.normal_index && lhs.vertex_index == rhs.vertex_index;
    }
};


bool hasSmoothedNormals(const tinyobj::shape_t& mesh)
{
    for (const uint32& smoothingGrpIdx : mesh.mesh.smoothing_group_ids)
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
void calcTangent(MeshLoaderData& loaderData, StaticMeshVertex& vertexData, const StaticMeshVertex& other1, const StaticMeshVertex& other2)
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
        Logger::error("StaticMeshLoader", "%s(): Incorrect texture coordinate, using world x, y as tangents", __func__);

        Rotation tbnFrame = RotationMatrix::fromZ(normal).asRotation();
        tangent = tbnFrame.fwdVector();
        bitangent = tbnFrame.rightVector();
    }
    else
    {
        invDet = 1 / invDet;

        tangent = invDet * (uv20.y() * p10 - uv10.y() * p20);
        // Gram-Schmidt orthogonalize
        tangent = (tangent - (tangent | normal) * normal).normalized();

        bitangent = invDet * (uv10.x() * p20 - uv20.x() * p10);
        // Gram-Schmidt orthogonalize
        bitangent = (bitangent - (bitangent | normal) * normal - (bitangent | tangent) * tangent).normalized();

        //
        // Handedness - dot(cross(normal(z) ^ tangent(x)), bitangent) must be positive
        //
        if (((normal ^ tangent) | bitangent) < 0)
        {
            tangent = -tangent;
        }
    }

    //vertexData.bitangent = Vector4D(bitangent, 0);
    vertexData.tangent = Vector4D(tangent, 0);

#if _DEBUG
    const float drawLen = 10;
    TbnLinePoint pt1;
    pt1.position = Vector3D(vertexData.position);
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
#endif
}

void fillVertexInfo(StaticMeshVertex& vertexData, const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
    // Inverting Y since UV origin is at left bottom of image and Graphics API's UV origin is at left top
    Vector2D uvCoord{ attrib.texcoords[index.texcoord_index * 2 + 0], (1.0f - attrib.texcoords[index.texcoord_index * 2 + 1]) };
    uvCoord = Math::clamp(uvCoord, Vector2D::ZERO, Vector2D::ONE);

    vertexData.position = Vector4D(
        attrib.vertices[index.vertex_index * 3]
        , attrib.vertices[index.vertex_index * 3 + 1]
        , attrib.vertices[index.vertex_index * 3 + 2]
        , uvCoord.x());
    Vector3D normal { 
        attrib.normals[index.normal_index * 3]
        , attrib.normals[index.normal_index * 3 + 1]
        , attrib.normals[index.normal_index * 3 + 2]
    };

    vertexData.normal = Vector4D(normal.normalized(), uvCoord.y());
    //vertexData.vertexColor = Vector4D(attrib.colors[index.vertex_index * 3], attrib.colors[index.vertex_index * 3 + 1]
    //    , attrib.colors[index.vertex_index * 3 + 2], 1.0f);
}

Vector3D StaticMeshLoader::getFaceNormal(const uint32& index0, const uint32& index1, const uint32& index2, const std::vector<StaticMeshVertex>& verticesData) const
{
    Vector4D temp1 = verticesData[index1].position - verticesData[index0].position;
    Vector4D temp2 = verticesData[index2].position - verticesData[index0].position;

    Vector3D dir1(temp1.x(), temp1.y(), temp1.z());
    Vector3D dir2(temp2.x(), temp2.y(), temp2.z());
    
    return (dir1 ^ dir2).normalized();
}

void StaticMeshLoader::addNormal(StaticMeshVertex& vertex, Vector3D& normal) const
{
    Vector4D& encodedNormal = vertex.normal;
    encodedNormal.x() += normal.x();
    encodedNormal.y() += normal.y();
    encodedNormal.z() += normal.z();
}

void StaticMeshLoader::normalize(Vector4D& normal) const
{
    Vector3D newNormal(normal);
    newNormal = newNormal.normalized();
    normal.x() = newNormal.x();
    normal.y() = newNormal.y();
    normal.z() = newNormal.z();
}

void StaticMeshLoader::load(const tinyobj::shape_t& mesh, const tinyobj::attrib_t& attrib)
{
    MeshLoaderData& meshLoaderData = loadedMeshes[mesh.name];
    meshLoaderData.indices.resize(mesh.mesh.indices.size());

    uint32 faceCount = uint32(mesh.mesh.indices.size() / 3);

    std::vector<int32> faceMaterialId(faceCount);
    std::set<int32> uniqueMatIds;
    // Vertices pushed to meshLoaderData along with indices
    {
        std::unordered_map<tinyobj::index_t, uint32> indexToNewVert;
        for (uint32 faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            const tinyobj::index_t& idx0 = mesh.mesh.indices[faceIdx * 3 + 0];
            const tinyobj::index_t& idx1 = mesh.mesh.indices[faceIdx * 3 + 1];
            const tinyobj::index_t& idx2 = mesh.mesh.indices[faceIdx * 3 + 2];
            uint32 newVertIdx0;
            uint32 newVertIdx1;
            uint32 newVertIdx2;

            faceMaterialId[faceIdx] = mesh.mesh.material_ids[faceIdx];
            uniqueMatIds.insert(mesh.mesh.material_ids[faceIdx]);

            // Filling vertex data to mesh struct
            {
                auto idx0NewVertItr = indexToNewVert.find(idx0);
                auto idx1NewVertItr = indexToNewVert.find(idx1);
                auto idx2NewVertItr = indexToNewVert.find(idx2);

                if (idx0NewVertItr == indexToNewVert.end())
                {
                    uint32& vertexIdx = indexToNewVert[idx0];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdx0 = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idx0);
                    meshLoaderData.bound.grow(Vector3D(meshLoaderData.vertices[vertexIdx].position));
                }
                else
                {
                    newVertIdx0 = idx0NewVertItr->second;
                }
                if (idx1NewVertItr == indexToNewVert.end())
                {
                    uint32& vertexIdx = indexToNewVert[idx1];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdx1 = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idx1);
                    meshLoaderData.bound.grow(Vector3D(meshLoaderData.vertices[vertexIdx].position));
                }
                else
                {
                    newVertIdx1 = idx1NewVertItr->second;
                }
                if (idx2NewVertItr == indexToNewVert.end())
                {
                    uint32& vertexIdx = indexToNewVert[idx2];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdx2 = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idx2);
                    meshLoaderData.bound.grow(Vector3D(meshLoaderData.vertices[vertexIdx].position));
                }
                else
                {
                    newVertIdx2 = idx2NewVertItr->second;
                }

                // Since we need all three vertex for tangent and bi-tangent calculations
                if (idx0NewVertItr == indexToNewVert.end())
                {
                    calcTangent(meshLoaderData, meshLoaderData.vertices[newVertIdx0]
                        , meshLoaderData.vertices[newVertIdx1]
                        , meshLoaderData.vertices[newVertIdx2]);
                }
                if (idx1NewVertItr == indexToNewVert.end())
                {
                    calcTangent(meshLoaderData, meshLoaderData.vertices[newVertIdx1]
                        , meshLoaderData.vertices[newVertIdx2]
                        , meshLoaderData.vertices[newVertIdx0]);
                }
                if (idx2NewVertItr == indexToNewVert.end())
                {
                    calcTangent(meshLoaderData, meshLoaderData.vertices[newVertIdx2]
                        , meshLoaderData.vertices[newVertIdx0]
                        , meshLoaderData.vertices[newVertIdx1]);
                }
            }

            //makeCCW(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);

            meshLoaderData.indices[faceIdx * 3 + 0] = newVertIdx0;
            meshLoaderData.indices[faceIdx * 3 + 1] = newVertIdx1;
            meshLoaderData.indices[faceIdx * 3 + 2] = newVertIdx2;
        }
    }

    splitMeshBatches(meshLoaderData, faceMaterialId, uint32(uniqueMatIds.size()), faceCount);

    // Normalizing all the vertex normals
    for (StaticMeshVertex& vertex : meshLoaderData.vertices)
    {
        normalize(vertex.normal);
    }
}

void StaticMeshLoader::smoothAndLoad(const tinyobj::shape_t& mesh, const tinyobj::attrib_t& attrib)
{
    const float smoothingThreshold = Math::cos(Math::deg2Rad(smoothingAngle));
    MeshLoaderData& meshLoaderData = loadedMeshes[mesh.name];
    meshLoaderData.indices.resize(mesh.mesh.indices.size());

    uint32 faceCount = uint32(mesh.mesh.indices.size() / 3);

    std::vector<int32> faceMaterialId(faceCount);
    std::set<int32> uniqueMatIds;
    // Vertices pushed to meshLoaderData along with indices
    {
        std::unordered_map<tinyobj::index_t, uint32> indexToNewVert;
        std::vector<std::unordered_map<uint32, std::vector<uint32>>> vertexFaceAdjacency;
        std::vector<Vector3D> faceNormals(faceCount);
        std::vector<uint32> faceSmoothingId(faceCount);

        for (uint32 faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            const tinyobj::index_t& idx0 = mesh.mesh.indices[faceIdx * 3 + 0];
            const tinyobj::index_t& idx1 = mesh.mesh.indices[faceIdx * 3 + 1];
            const tinyobj::index_t& idx2 = mesh.mesh.indices[faceIdx * 3 + 2];
            uint32 newVertIdx0;
            uint32 newVertIdx1;
            uint32 newVertIdx2;

            faceSmoothingId[faceIdx] = mesh.mesh.smoothing_group_ids[faceIdx];
            faceMaterialId[faceIdx] = mesh.mesh.material_ids[faceIdx];
            uniqueMatIds.insert(mesh.mesh.material_ids[faceIdx]);

            // Filling vertex data to mesh struct
            {
                auto idx0NewVertItr = indexToNewVert.find(idx0);
                auto idx1NewVertItr = indexToNewVert.find(idx1);
                auto idx2NewVertItr = indexToNewVert.find(idx2);

                if (idx0NewVertItr == indexToNewVert.end())
                {
                    uint32& vertexIdx = indexToNewVert[idx0];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdx0 = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    vertexFaceAdjacency.push_back({});
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idx0);
                    meshLoaderData.bound.grow(Vector3D(meshLoaderData.vertices[vertexIdx].position));
                }
                else
                {
                    newVertIdx0 = idx0NewVertItr->second;
                }
                if (idx1NewVertItr == indexToNewVert.end())
                {
                    uint32& vertexIdx = indexToNewVert[idx1];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdx1 = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    vertexFaceAdjacency.push_back({});
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idx1);
                    meshLoaderData.bound.grow(Vector3D(meshLoaderData.vertices[vertexIdx].position));
                }
                else
                {
                    newVertIdx1 = idx1NewVertItr->second;
                }
                if (idx2NewVertItr == indexToNewVert.end())
                {
                    uint32& vertexIdx = indexToNewVert[idx2];
                    vertexIdx = uint32(meshLoaderData.vertices.size());

                    newVertIdx2 = vertexIdx;
                    meshLoaderData.vertices.push_back(StaticMeshVertex());
                    vertexFaceAdjacency.push_back({});
                    fillVertexInfo(meshLoaderData.vertices[vertexIdx], attrib, idx2);
                    meshLoaderData.bound.grow(Vector3D(meshLoaderData.vertices[vertexIdx].position));
                }
                else
                {
                    newVertIdx2 = idx2NewVertItr->second;
                }

                // Since we need all three vertex for tangent and bi-tangent calculations
                if (idx0NewVertItr == indexToNewVert.end())
                {
                    calcTangent(meshLoaderData, meshLoaderData.vertices[newVertIdx0]
                        , meshLoaderData.vertices[newVertIdx1]
                        , meshLoaderData.vertices[newVertIdx2]);
                }
                if (idx1NewVertItr == indexToNewVert.end())
                {
                    calcTangent(meshLoaderData, meshLoaderData.vertices[newVertIdx1]
                        , meshLoaderData.vertices[newVertIdx2]
                        , meshLoaderData.vertices[newVertIdx0]);
                }
                if (idx2NewVertItr == indexToNewVert.end())
                {
                    calcTangent(meshLoaderData, meshLoaderData.vertices[newVertIdx2]
                        , meshLoaderData.vertices[newVertIdx0]
                        , meshLoaderData.vertices[newVertIdx1]);
                }
            }

            //makeCCW(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);

            meshLoaderData.indices[faceIdx * 3 + 0] = newVertIdx0;
            meshLoaderData.indices[faceIdx * 3 + 1] = newVertIdx1;
            meshLoaderData.indices[faceIdx * 3 + 2] = newVertIdx2;

            faceNormals[faceIdx] = getFaceNormal(newVertIdx0, newVertIdx1, newVertIdx2, meshLoaderData.vertices);

            // Fill vertex pair's(Edge's) faces adjacency
            {
                std::unordered_map<uint32, std::vector<uint32>>::iterator diagonalItr;
                // Vertex 0
                {
                    diagonalItr = vertexFaceAdjacency[newVertIdx0].find(newVertIdx1);
                    if (diagonalItr != vertexFaceAdjacency[newVertIdx0].end())
                    {
                        diagonalItr->second.push_back(faceIdx);
                    }
                    else
                    {
                        vertexFaceAdjacency[newVertIdx0][newVertIdx1].push_back(faceIdx);
                    }

                    diagonalItr = vertexFaceAdjacency[newVertIdx0].find(newVertIdx2);
                    if (diagonalItr != vertexFaceAdjacency[newVertIdx0].end())
                    {
                        diagonalItr->second.push_back(faceIdx);
                    }
                    else
                    {
                        vertexFaceAdjacency[newVertIdx0][newVertIdx2].push_back(faceIdx);
                    }
                }
                // Vertex 1
                {
                    diagonalItr = vertexFaceAdjacency[newVertIdx1].find(newVertIdx0);
                    if (diagonalItr != vertexFaceAdjacency[newVertIdx1].end())
                    {
                        diagonalItr->second.push_back(faceIdx);
                    }
                    else
                    {
                        vertexFaceAdjacency[newVertIdx1][newVertIdx0].push_back(faceIdx);
                    }

                    diagonalItr = vertexFaceAdjacency[newVertIdx1].find(newVertIdx2);
                    if (diagonalItr != vertexFaceAdjacency[newVertIdx1].end())
                    {
                        diagonalItr->second.push_back(faceIdx);
                    }
                    else
                    {
                        vertexFaceAdjacency[newVertIdx1][newVertIdx2].push_back(faceIdx);
                    }
                }
                // Vertex 2
                {
                    diagonalItr = vertexFaceAdjacency[newVertIdx2].find(newVertIdx1);
                    if (diagonalItr != vertexFaceAdjacency[newVertIdx2].end())
                    {
                        diagonalItr->second.push_back(faceIdx);
                    }
                    else
                    {
                        vertexFaceAdjacency[newVertIdx2][newVertIdx1].push_back(faceIdx);
                    }

                    diagonalItr = vertexFaceAdjacency[newVertIdx2].find(newVertIdx0);
                    if (diagonalItr != vertexFaceAdjacency[newVertIdx2].end())
                    {
                        diagonalItr->second.push_back(faceIdx);
                    }
                    else
                    {
                        vertexFaceAdjacency[newVertIdx2][newVertIdx0].push_back(faceIdx);
                    }
                }
            }
        }

        uint32 originalVertCount = uint32(meshLoaderData.vertices.size());

        for (uint32 vertIdx = 0; vertIdx < originalVertCount; ++vertIdx)
        {
            std::vector<std::set<uint32>> faceGroups;

            for (const std::pair<const uint32, std::vector<uint32>>& adjacentFaces : vertexFaceAdjacency[vertIdx])
            {
                float dotVal = 1;
                bool isSameSmoothing = true;
                if (adjacentFaces.second.size() == 2)
                {
                    dotVal = faceNormals[adjacentFaces.second[0]] | faceNormals[adjacentFaces.second[1]];
                    isSameSmoothing = faceSmoothingId[adjacentFaces.second[0]] == faceSmoothingId[adjacentFaces.second[1]];
                }

                if (dotVal >= smoothingThreshold && isSameSmoothing) // Is same face group
                {
                    std::set<uint32> faceIndices;
                    faceIndices.insert(adjacentFaces.second.begin(), adjacentFaces.second.end());

                    std::set<uint32> faceGrpsFound;
                    for (uint32 faceGrpIdx = 0; faceGrpIdx < faceGroups.size(); ++faceGrpIdx)
                    {
                        std::vector<uint32> intersectSet(Math::min(faceIndices.size(), faceGroups[faceGrpIdx].size()));
                        auto intersectItr = std::set_intersection(faceIndices.cbegin(), faceIndices.cend(), faceGroups[faceGrpIdx].cbegin(), faceGroups[faceGrpIdx].cend()
                            , intersectSet.begin());
                        if ((intersectItr - intersectSet.begin()) > 0)// If any face idx is already in group add to that group
                        {
                            faceGrpsFound.insert(faceGrpIdx);
                        }
                    }

                    if (faceGrpsFound.size() == 0)
                    {
                        faceGroups.push_back(faceIndices);
                    }
                    else if (faceGrpsFound.size() == 1)
                    {
                        faceGroups[*faceGrpsFound.begin()].insert(faceIndices.begin(), faceIndices.end());
                    }
                    else
                    {
                        auto faceGrpIdxItr = --faceGrpsFound.end();
                        do
                        {
                            faceGroups[*faceGrpsFound.begin()].insert(faceGroups[*faceGrpIdxItr].begin(), faceGroups[*faceGrpIdxItr].end());
                            faceGroups.erase(faceGroups.begin() + *faceGrpIdxItr);
                            --faceGrpIdxItr;
                        } while (faceGrpIdxItr != faceGrpsFound.begin());

                        faceGroups[*faceGrpsFound.begin()].insert(faceIndices.begin(), faceIndices.end());
                    }
                }
                else // Non smoothing cases happen only if there is 2 adjacent faces
                {
                    {// For adjacent face at 0
                        bool bIsInserted = false;
                        for (uint32 faceGrpIdx = 0; faceGrpIdx < faceGroups.size(); ++faceGrpIdx)
                        {
                            auto faceItr = faceGroups[faceGrpIdx].find(adjacentFaces.second[0]);
                            if (faceItr != faceGroups[faceGrpIdx].end())
                            {
                                bIsInserted = true;
                            }
                        }
                        if (!bIsInserted)
                        {
                            faceGroups.push_back({ adjacentFaces.second[0] });
                        }
                    }
                    {// For adjacent face at 1
                        bool bIsInserted = false;
                        for (uint32 faceGrpIdx = 0; faceGrpIdx < faceGroups.size(); ++faceGrpIdx)
                        {
                            auto faceItr = faceGroups[faceGrpIdx].find(adjacentFaces.second[1]);
                            if (faceItr != faceGroups[faceGrpIdx].end())
                            {
                                bIsInserted = true;
                            }
                        }
                        if (!bIsInserted)
                        {
                            faceGroups.push_back({ adjacentFaces.second[1] });
                        }
                    }
                }
            }

            auto faceGrpsItr = faceGroups.begin();
            for (const uint32& faceIdx : *faceGrpsItr)
            {
                uint32 faceStartIndex = faceIdx * 3;
                for (uint32 i = 0; i < 3; ++i)
                {
                    if (vertIdx == meshLoaderData.indices[faceStartIndex + i])
                    {
                        addNormal(meshLoaderData.vertices[vertIdx], faceNormals[faceIdx]);
                        break;
                    }
                }
            }
            ++faceGrpsItr;
            while (faceGrpsItr != faceGroups.end())
            {
                uint32 newVertIndex = uint32(meshLoaderData.vertices.size());
                meshLoaderData.vertices.push_back(meshLoaderData.vertices[vertIdx]);

                for (const uint32& faceIdx : *faceGrpsItr)
                {
                    uint32 faceStartIndex = faceIdx * 3;
                    for (uint32 i = 0; i < 3; ++i)
                    {
                        if (vertIdx == meshLoaderData.indices[faceStartIndex + i])
                        {
                            meshLoaderData.indices[faceStartIndex + i] = newVertIndex;
                            addNormal(meshLoaderData.vertices[newVertIndex], faceNormals[faceIdx]);
                            break;
                        }
                    }
                }
                ++faceGrpsItr;
            }
        }
    }

    splitMeshBatches(meshLoaderData, faceMaterialId, uint32(uniqueMatIds.size()), faceCount);

    // Normalizing all the vertex normals
    for (StaticMeshVertex& vertex : meshLoaderData.vertices)
    {
        normalize(vertex.normal);
    }
}

void StaticMeshLoader::splitMeshBatches(MeshLoaderData& meshLoaderData, const std::vector<int32> &faceMaterialId, uint32 uniqueMatCount, uint32 faceCount)
{
    // Splitting based on face material IDs
    if (uniqueMatCount > 1)
    {
        std::unordered_map<uint32, std::vector<uint32>> materialIdToIndices;

        for (uint32 faceIdx = 0; faceIdx < faceCount; ++faceIdx)
        {
            std::vector<uint32>& indices = materialIdToIndices[faceMaterialId[faceIdx]];

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
        for (const std::pair<const uint32, std::vector<uint32>>& matIdIndices : materialIdToIndices)
        {
            MeshVertexView vertexBatchView;
            vertexBatchView.startIndex = uint32(meshLoaderData.indices.size());
            vertexBatchView.numOfIndices = uint32(matIdIndices.second.size());
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

StaticMeshLoader::StaticMeshLoader(const String& assetPath)
    : bIsSuccessful(false)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> meshes;
    std::vector<tinyobj::material_t> materials;

    String warning("");
    String error("");
    bIsSuccessful = tinyobj::LoadObj(&attrib, &meshes, &materials, &warning, &error, assetPath.getChar()
        , PlatformFile(assetPath).getHostDirectory().getChar());
    warning.trim();
    error.trim();
    if (!warning.empty())
    {
        Logger::warn("StaticMeshLoader", "Tiny obj loader %s", warning.getChar());
    }
    if (!error.empty())
    {
        Logger::error("StaticMeshLoader", "Tiny obj loader %s", error.getChar());
        return;
    }

    for (tinyobj::shape_t& mesh : meshes)
    {
        const bool hasSmoothing = hasSmoothedNormals(mesh);

        if (bLoadSmoothed && !hasSmoothing)
        {
            smoothAndLoad(mesh, attrib);
        }
        else
        {
            load(mesh, attrib);
        }
    }
}
#undef TINYOBJLOADER_IMPLEMENTATION

bool StaticMeshLoader::fillAssetInformation(const std::vector<StaticMeshAsset*>& assets) const
{
    if (bIsSuccessful)
    {
        uint32 idx = 0;
        for (const std::pair<const String, MeshLoaderData>& meshDataPair : loadedMeshes)
        {
            StaticMeshAsset* staticMesh = assets[idx];
            staticMesh->setAssetName(meshDataPair.first);
            staticMesh->vertices = meshDataPair.second.vertices;
            staticMesh->indices = meshDataPair.second.indices;
            staticMesh->meshBatches = meshDataPair.second.meshBatches;
            staticMesh->bounds = meshDataPair.second.bound;

#if _DEBUG
            staticMesh->tbnVerts = meshDataPair.second.tbnVerts;
#endif
            ++idx;
        }
    }
    return bIsSuccessful;
}


uint32 StaticMeshLoader::getMeshNum() const
{
    return uint32(loadedMeshes.size());
}

void AssetLoaderLibrary::loadStaticMesh(const String& assetPath, std::vector<AssetBase*>& staticMeshes)
{
    StaticMeshLoader loader(assetPath);
    std::vector<StaticMeshAsset*> meshes(loader.getMeshNum());

    for (StaticMeshAsset*& mesh : meshes)
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
    for (StaticMeshAsset*& mesh : meshes)
    {
        delete mesh;
    }
    meshes.clear();
    staticMeshes.clear();
}
