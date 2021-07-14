#pragma once
#include "../../../Core/Math/Vector4D.h"
#include "../../../Core/Math/Vector3D.h"
#include "../../../Core/Math/Vector2D.h"

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