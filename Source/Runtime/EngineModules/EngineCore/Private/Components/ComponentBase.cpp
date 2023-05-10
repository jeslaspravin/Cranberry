/*!
 * \file ComponentBase.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Components/ComponentBase.h"
#include "CBEObjectHelpers.h"
#include "Classes/Actor.h"
#include "Classes/World.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
/// LogicComponent implementation
//////////////////////////////////////////////////////////////////////////

World *LogicComponent::getWorld() const { return getActor()->getWorld(); }

//////////////////////////////////////////////////////////////////////////
/// TransformComponent implementation
//////////////////////////////////////////////////////////////////////////

World *TransformComponent::getWorld() const
{
    if (Actor *actor = getActor())
    {
        return actor->getWorld();
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
/// TransformLeafComponent implementation
//////////////////////////////////////////////////////////////////////////

World *TransformLeafComponent::getWorld() const
{
    if (Actor *actor = getActor())
    {
        return actor->getWorld();
    }
    return nullptr;
}

} // namespace cbe
