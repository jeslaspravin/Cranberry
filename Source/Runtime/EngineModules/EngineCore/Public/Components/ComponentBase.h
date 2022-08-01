/*!
 * \file ComponentBase.h
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

#include "ComponentBase.gen.h"

namespace cbe
{
class Actor;
class World;

/*!
 * \class LogicComponent
 *
 * \brief Logic component is used to add logics to actor that it belongs to. This component do not have any transforms and will not be present
 * in transform hierarchy of actor components
 *
 * \author Jeslas
 * \date July 2022
 */
class ENGINECORE_EXPORT LogicComponent : public Object
{
    GENERATED_CODES()

public:
    World *getWorld() const;
    Actor *getActor() const;

} META_ANNOTATE(NoExport);

/*!
 * \class TransformComponent
 *
 * \brief Transform component will be the base every other component that can be moved in level. I do not have any visuals on its own
 *
 * \author Jeslas
 * \date July 2022
 */
class ENGINECORE_EXPORT TransformComponent : public Object
{
    GENERATED_CODES()

public:
    META_ANNOTATE()
    Transform3D relativeTf;

private:
    META_ANNOTATE(ObjFlag_Transient)
    TransformComponent *attachedTo = nullptr;

public:
    void attachComponent(TransformComponent *otherComp);
    void detachComponent();
    void setAttachedTo(TransformComponent *otherComp) { attachedTo = otherComp; }
    TransformComponent *getAttachedTo() const { return attachedTo; }

    World *getWorld() const;
    Actor *getActor() const;

} META_ANNOTATE(NoExport);

} // namespace cbe
