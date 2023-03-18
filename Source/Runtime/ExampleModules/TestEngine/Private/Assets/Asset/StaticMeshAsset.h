/*!
 * \file StaticMeshAsset.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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