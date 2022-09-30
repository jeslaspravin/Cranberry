/*!
 * \file EngineRenderTypes.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "RenderApi/VertexData.h"

class Camera;
class IRenderCommandList;
class GraphicsHelperAPI;
class IGraphicsInstance;

namespace cbe
{
class TransformComponent;
class RenderableComponent;
class World;
} // namespace cbe

struct ComponentRenderSyncInfo
{
    std::vector<StringID> compsRemoved;
    std::vector<cbe::RenderableComponent *> compsAdded;
    std::vector<cbe::RenderableComponent *> recreateComps;
    std::vector<cbe::TransformComponent *> compsTransformed;

    void clear()
    {
        compsRemoved.clear();
        compsAdded.clear();
        recreateComps.clear();
        compsTransformed.clear();
    }
};