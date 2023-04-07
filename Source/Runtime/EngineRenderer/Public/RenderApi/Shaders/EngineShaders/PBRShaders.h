/*!
 * \file PBRShaders.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/Matrix4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

struct PbrSpotLight
{
    Vector4 sptLightColor_lumen; // Color and lumen
    Vector4 sptPos_radius;       // Position and radius
    Vector4 sptDirection;        // empty w
    Vector2 sptCone;             // inner, outer cone
};

struct PbrPointLight
{
    Vector4 ptLightColor_lumen; // Color and lumen
    Vector4 ptPos_radius;       // Position and radius
};

struct PbrDirectionalLight
{
    Vector4 lightColor_lumen;
    Vector3 direction;
};

struct PBRLightArray
{
    uint32 count;

    PbrSpotLight spotLits[8];
    PbrPointLight ptLits[8];
    PbrDirectionalLight dirLit;
};

struct ColorCorrection
{
    float exposure;
    float gamma;
};

namespace PBRShadowFlags
{
enum Flags
{
    DrawingBackface = 1,
};
} // namespace PBRShadowFlags

struct ShadowData
{
    // World to clip
    Matrix4 sptLitsW2C[8];
    // Max 8 cascades
    Matrix4 dirLitCascadesW2C[8];
    // Far distance for each cascade
    float cascadeFarPlane[8];
    uint32 shadowFlags;
};