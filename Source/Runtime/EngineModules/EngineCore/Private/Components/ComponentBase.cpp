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

void TransformComponent::attachComponent(TransformComponent *otherComp)
{
    if (attachedTo != otherComp)
    {
        attachedTo = otherComp;
        getWorld()->onAttachmentChanged(this, otherComp);
    }
}

void TransformComponent::detachComponent()
{
    if (attachedTo)
    {
        attachedTo = nullptr;
        getWorld()->onAttachmentChanged(this, nullptr);
    }
}

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
} // namespace cbe
