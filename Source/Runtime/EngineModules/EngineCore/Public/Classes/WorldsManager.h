/*!
 * \file WorldsManager.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "CBEObject.h"

#if __REF_PARSE__
#include "Classes/World.h"
#endif

#include "WorldsManager.gen.h"

namespace cbe
{

class World;

class WorldsManager : public Object
{
    GENERATED_CODES()
public:
    constexpr static const uint32 AllocSlotCount = 2;

private:
    META_ANNOTATE()
    World *mainWorld;

} META_ANNOTATE();

} // namespace cbe