/*!
 * \file GameEngine.h
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

#include "GameEngine.gen.h"

namespace CBE
{

class META_ANNOTATE_API(ENGINECORE_EXPORT) GameEngine : public Object
{
    GENERATED_CODES()
public:
    // Overriding Allocator slot count to 2 as 2 is enough to be allocated at any time for GameEngine
    constexpr static const uint32 AllocSlotCount = 2;

public:
};

} // namespace CBE