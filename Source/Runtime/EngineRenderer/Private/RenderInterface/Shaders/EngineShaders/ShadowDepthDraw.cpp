/*!
 * \file ShadowDepthDraw.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Shaders/EngineShaders/ShadowDepthDraw.h"
#include "Math/RotationMatrix.h"
#include "Math/Vector3D.h"

BEGIN_BUFFER_DEFINITION(PointShadowDepthViews)
ADD_BUFFER_TYPED_FIELD(w2Clip)
ADD_BUFFER_TYPED_FIELD(lightPosFarPlane)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(DirectionalShadowCascadeViews)
ADD_BUFFER_TYPED_FIELD(cascadeW2Clip)
ADD_BUFFER_TYPED_FIELD(cascadeCount)
END_BUFFER_DEFINITION();

ShaderBufferParamInfo *DirectionalShadowCascadeViews::paramInfo()
{
    static DirectionalShadowCascadeViewsBufferParamInfo PARAM_INFO;
    return &PARAM_INFO;
}

ShaderBufferParamInfo *PointShadowDepthViews::paramInfo()
{
    static PointShadowDepthViewsBufferParamInfo PARAM_INFO;
    return &PARAM_INFO;
}

// Doing explicit negation over unary negation to avoid unwanted rotation when doing atan2
Rotation PointShadowDepthViews::VIEW_DIRECTIONS[6]
    = { RotationMatrix::fromZX(Vector3D::UP, Vector3D::RIGHT).asRotation(),
        RotationMatrix::fromZX(Vector3D::UP, Vector3D(0, -1, 0) /*-Vector3D::RIGHT*/).asRotation(),
        RotationMatrix::fromZX(Vector3D(-1, 0, 0) /*-Vector3D::FWD*/, Vector3D::UP).asRotation(),
        RotationMatrix::fromZX(Vector3D::FWD, Vector3D(0, 0, -1) /*-Vector3D::UP*/).asRotation(),
        RotationMatrix::fromZX(Vector3D::UP, Vector3D::FWD).asRotation(),
        RotationMatrix::fromZX(Vector3D::UP, Vector3D(-1, 0, 0) /*-Vector3D::FWD*/).asRotation() };
