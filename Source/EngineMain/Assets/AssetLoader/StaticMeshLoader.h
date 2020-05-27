#pragma once

#include "../../Core/Platform/PlatformTypes.h"

#include <vector>
#include <map>

class String;
class StaticMeshAsset;
class Vector3D;
class Vector4D;
struct StaticMeshVertex;

/*
* Mesh tri winding will be CW from DCC, Since our view is LH coord the y gets inverted to make winding CCW, which in turn becomes CW on screen
*/
class StaticMeshLoader
{
private:
    bool bIsSuccessful;
    float smoothingAngle = 35.0f;// TODO(Jeslas) : expose this later for more controllable loading
    
    std::map<String, struct MeshLoaderData> loadedMeshes;

    Vector3D getFaceNormal(const uint32& index0, const uint32& index1, const uint32& index2, const std::vector<StaticMeshVertex>& verticesData) const;
    void addNormal(StaticMeshVertex& vertex, Vector3D& normal) const;
    void normalize(Vector4D& normal) const;
public:
    StaticMeshLoader(const String& assetPath);

    bool fillAssetInformation(const std::vector<StaticMeshAsset*>& assets) const;
    uint32 getMeshNum() const;
};