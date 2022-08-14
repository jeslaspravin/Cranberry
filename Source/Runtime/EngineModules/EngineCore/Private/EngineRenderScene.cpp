/*!
 * \file EngineRenderScene.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EngineRenderScene.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"

copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread> EngineRenderScene::syncWorldComps(ComponentSyncInfo compsUpdate)
{
    for (StringID compToRemove : compsUpdate.compsRemoved)
    {
        auto compToIdxItr = componentToRenderInfo.find(compToRemove);
        if (compToIdxItr != componentToRenderInfo.end())
        {
            SizeT renderInfoIdx = compToIdxItr->second;
            componentToRenderInfo.erase(compToIdxItr);

            destroyRenderInfo(compsRenderInfo[renderInfoIdx]);
            compsRenderInfo.reset(renderInfoIdx);
        }
    }

    // TODO :
    co_return;
}

void EngineRenderScene::clearScene() {}

void EngineRenderScene::createRenderInfo(cbe::RenderableComponent *comp, ComponentRenderInfo &outRenderInfo) const
{
    // TODO :
}

void EngineRenderScene::destroyRenderInfo(const ComponentRenderInfo &renderInfo) const
{
    // TODO :
}
