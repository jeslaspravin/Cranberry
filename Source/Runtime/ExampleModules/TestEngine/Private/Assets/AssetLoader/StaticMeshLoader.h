/*!
 * \file StaticMeshLoader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"

#include <map>
#include <vector>

class String;
class StaticMeshAsset;
class Vector3;
class Vector4;
struct StaticMeshVertex;

namespace tinyobj
{
struct material_t;
}
namespace tinyobj
{
struct attrib_t;
}
namespace tinyobj
{
struct shape_t;
}

/*
 * Mesh tri winding will be CW from DCC, Since our view is LH coord the y gets inverted to make winding
 * CCW, which in turn becomes CW on screen
 */
class StaticMeshLoader
{
private:
    constexpr static const uint32 FACE_MAX_VERTS = 3;

    bool bIsSuccessful;
    // TODO(Jeslas) : expose this later for more controllable loading
    bool bLoadSmoothed = false;
    float smoothingAngle = 35.0f;

    std::map<String, struct MeshLoaderData> loadedMeshes;

    Vector3 getFaceNormal(uint32 index0, uint32 index1, uint32 index2, const std::vector<StaticMeshVertex> &verticesData) const;
    void addNormal(StaticMeshVertex &vertex, Vector3 &normal) const;
    void normalize(Vector4 &normal) const;

    void load(const tinyobj::shape_t &mesh, const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials);
    void smoothAndLoad(const tinyobj::shape_t &mesh, const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials);
    // Splits loaded mesh into batches based on material IDs
    void splitMeshBatches(
        MeshLoaderData &meshLoaderData, const std::vector<int32> &faceMaterialId, const std::vector<tinyobj::material_t> &materials,
        uint32 uniqueMatCount, uint32 faceCount
    );

public:
    StaticMeshLoader(const String &assetPath);

    bool fillAssetInformation(const std::vector<StaticMeshAsset *> &assets) const;
    uint32 getMeshNum() const;
};