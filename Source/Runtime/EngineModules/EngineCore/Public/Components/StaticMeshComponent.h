/*!
 * \file StaticMeshComponent.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Components/RenderableComponent.h"

#if __REF_PARSE__
#include "Classes/StaticMesh.h"
#endif
#include "StaticMeshComponent.gen.h"

namespace cbe
{
class StaticMesh;

class ENGINECORE_EXPORT StaticMeshComponent : public RenderableComponent
{
    GENERATED_CODES()
public:
    META_ANNOTATE()
    StaticMesh *mesh;

    /* RenderableComponent overrides */
    void setupRenderInfo(ComponentRenderInfo &compRenderInfo) const override;
    void clearRenderInfo(const ComponentRenderInfo &compRenderInfo) const override;
    /* Override ends */
} META_ANNOTATE(NoExport);

} // namespace cbe