/*!
 * \file RenderableComponent.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Components/ComponentBase.h"

#include "RenderableComponent.gen.h"

struct ComponentRenderInfo;

namespace cbe
{

class ENGINECORE_EXPORT RenderableComponent : public TransformLeafComponent
{
    GENERATED_CODES()

public:
    virtual void setupRenderInfo(ComponentRenderInfo &compRenderInfo) const = 0;
    virtual void clearRenderInfo(const ComponentRenderInfo &compRenderInfo) const = 0;
    virtual AABB getLocalBound() const = 0;

} META_ANNOTATE(NoExport);

} // namespace cbe