/*!
 * \file EngineBase.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "CBEObject.h"
#include "CBEObjectHelpers.h"

#include "EngineBase.gen.h"

namespace cbe
{
class WorldsManager;

class ENGINECORE_EXPORT EngineBase : public Object
{
    GENERATED_CODES()
public:
    // Overriding Allocator slot count to 2 as 2 is enough to be allocated at any time for GameEngine
    constexpr static const uint32 AllocSlotCount = 2;

public:
    EngineBase();

    void onStart();
    void onTick();
    void onExit();

    WorldsManager *worldManager() const;

    virtual void engineStart() {}
    virtual void engineTick(float /*timeDelta*/) {}
    virtual void engineExit() {}
} META_ANNOTATE(NoExport);

} // namespace cbe

ENGINECORE_EXPORT extern cbe::EngineBase *gCBEEngine;