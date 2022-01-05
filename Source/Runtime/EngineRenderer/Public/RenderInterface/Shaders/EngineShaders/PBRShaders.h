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
#include "Math/Vector4D.h"
#include "Math/Vector3D.h"
#include "Math/Vector2D.h"
#include "Math/Matrix4.h"

struct PbrSpotLight
{
    Vector4D sptLightColor_lumen;// Color and lumen
    Vector4D sptPos_radius;// Position and radius
    Vector4D sptDirection;// empty w
    Vector2D sptCone;// inner, outer cone
};

struct PbrPointLight
{
    Vector4D ptLightColor_lumen;// Color and lumen
    Vector4D ptPos_radius;// Position and radius
};

struct PbrDirectionalLight
{
    Vector4D lightColor_lumen;
    Vector3D direction;
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
}

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