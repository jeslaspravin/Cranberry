/*!
 * \file StaticMeshComponent.cpp
 *
 * \author Jeslas
 * \date September 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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
    if (cbe::isValid(mesh))
    {
        compRenderInfo.cpuIdxBuffer = mesh->indexCpuBuffer;
        compRenderInfo.cpuVertBuffer = mesh->vertexCpuBuffer;

        compRenderInfo.vertexType = EVertexType::StaticMesh;
        compRenderInfo.meshID = mesh->getStringID();
        compRenderInfo.worldTf = getWorldTransform();
    }
}

void StaticMeshComponent::clearRenderInfo(const ComponentRenderInfo &compRenderInfo) const {}

} // namespace cbe