#pragma once
#include "MeshAsset.h"
#include "../../Core/Math/Vector4D.h"

struct StaticMeshVertex
{
    Vector4D position;// xyz position, w texture coord's U
    Vector4D normal;// xyz normal, w texture coord's V
    Vector4D vertexColor;
};

class StaticMeshAsset : public MeshAsset
{
public:
    std::vector<StaticMeshVertex> vertices;
    std::vector<uint32> indices;
    std::vector<MeshVertexView> meshBatches;
public:
    void initAsset() override;
    void clearAsset() override;
};