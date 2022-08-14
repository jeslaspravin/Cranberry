/*!
 * \file EngineRenderScene.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/StringID.h"
#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"

namespace cbe
{
class RenderableComponent;
}

// 1:1 to each world
class EngineRenderScene
{
private:
    struct ComponentRenderInfo
    {};

    SparseVector<ComponentRenderInfo, BitArraySparsityPolicy> compsRenderInfo;
    std::unordered_map<StringID, SizeT> componentToRenderInfo;

public:
    struct ComponentSyncInfo
    {
        std::vector<StringID> compsRemoved;
        std::vector<cbe::RenderableComponent *> compsAdded;
        std::vector<cbe::RenderableComponent *> recreateComps;
        std::vector<cbe::RenderableComponent *> compsTransformed;
    };
    copat::JobSystemEnqTask<copat::EJobThreadType::RenderThread> syncWorldComps(ComponentSyncInfo compsUpdate);

    void clearScene();

private:
    void createRenderInfo(cbe::RenderableComponent *comp, ComponentRenderInfo &outRenderInfo) const;
    void destroyRenderInfo(const ComponentRenderInfo &renderInfo) const;
};