#pragma once

#include "MeshAsset.h"
#include "RenderApi/VertexData.h"


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