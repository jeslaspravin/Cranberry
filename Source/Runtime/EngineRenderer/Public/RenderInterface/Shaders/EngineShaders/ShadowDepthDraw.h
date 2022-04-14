/*!
 * \file ShadowDepthDraw.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "EngineRendererExports.h"
#include "Math/Matrix4.h"
#include "Math/Rotation.h"
#include "Math/Vector4D.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"

// 0, 1 are matrices for view along +y and -y(Since they are internally considered X axis and it is Y
// axis in engine) 2, 3 are matrices for view along +z and -z(Since they are internally considered Y axis
// and it is Z axis in engine) 4, 5 are matrices for view along +x and -x(Since they are internally
// considered Z axis and it is X axis in engine)
struct PointShadowDepthViews
{
    Matrix4 w2Clip[6];
    Vector4D lightPosFarPlane;

    ENGINERENDERER_EXPORT static ShaderBufferParamInfo *paramInfo();
    ENGINERENDERER_EXPORT static Rotation VIEW_DIRECTIONS[6];
};

// Max 8 cascades
struct DirectionalShadowCascadeViews
{
    Matrix4 cascadeW2Clip[8];
    uint32 cascadeCount;

    ENGINERENDERER_EXPORT static ShaderBufferParamInfo *paramInfo();
};