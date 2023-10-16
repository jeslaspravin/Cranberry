/*!
 * \file StaticMeshComponent.cpp
 *
 * \author Jeslas
 * \date September 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Components/StaticMeshComponent.h"
#include "Classes/StaticMesh.h"
#include "EngineRenderScene.h"

namespace cbe
{

void StaticMeshComponent::setupRenderInfo(ComponentRenderInfo &compRenderInfo) const
{
    if (cbe::isValidFast(mesh))
    {
        compRenderInfo.cpuIdxBuffer = mesh->indexCpuBuffer;
        compRenderInfo.cpuVertBuffer = mesh->vertexCpuBuffer;

        compRenderInfo.vertexType = EVertexType::StaticMesh;
        compRenderInfo.meshObjPath = mesh;

        compRenderInfo.worldTf = getWorldTransform();
        compRenderInfo.worldBound = AABB();

        Vector3 aabbCorners[8];
        getLocalBound().boundCorners(aabbCorners);
        for (uint32 i = 0; i != ARRAY_LENGTH(aabbCorners); ++i)
        {
            compRenderInfo.worldBound.grow(compRenderInfo.worldTf.transformPoint(aabbCorners[i]));
        }
    }
}

void StaticMeshComponent::clearRenderInfo(const ComponentRenderInfo & /*compRenderInfo*/) const {}

AABB StaticMeshComponent::getLocalBound() const
{
    if (cbe::isValidFast(mesh))
    {
        return mesh->bounds;
    }
    return {};
}

} // namespace cbe