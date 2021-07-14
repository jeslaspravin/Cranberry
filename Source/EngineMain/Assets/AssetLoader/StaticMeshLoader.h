#pragma once

#include "../../Core/Platform/PlatformTypes.h"

#include <vector>
#include <map>

class String;
class StaticMeshAsset;
class Vector3D;
class Vector4D;
struct StaticMeshVertex;

namespace tinyobj { struct attrib_t; }
namespace tinyobj { struct shape_t; }

/*
* Mesh tri winding will be CW from DCC, Since our view is LH coord the y gets inverted to make winding CCW, which in turn becomes CW on screen
*/
class StaticMeshLoader
{
private:
    bool bIsSuccessful;
    // TODO(Jeslas) : expose this later for more controllable loading
    bool bLoadSmoothed = false;
    float smoothingAngle = 35.0f;
    
    std::map<String, struct MeshLoaderData> loadedMeshes;

    Vector3D getFaceNormal(const uint32& index0, const uint32& index1, const uint32& index2, const std::vector<StaticMeshVertex>& verticesData) const;
    void addNormal(StaticMeshVertex& vertex, Vector3D& normal) const;
    void normalize(Vector4D& normal) const;

    void load(const tinyobj::shape_t& mesh, const tinyobj::attrib_t& attrib);
    void smoothAndLoad(const tinyobj::shape_t& mesh, const tinyobj::attrib_t& attrib);
    // Splits loaded mesh into batches based on material IDs
    void splitMeshBatches(MeshLoaderData& meshLoaderData, const std::vector<int32> &faceMaterialId, uint32 uniqueMatCount, uint32 faceCount);
public:
    StaticMeshLoader(const String& assetPath);

    bool fillAssetInformation(const std::vector<StaticMeshAsset*>& assets) const;
    uint32 getMeshNum() const;
};