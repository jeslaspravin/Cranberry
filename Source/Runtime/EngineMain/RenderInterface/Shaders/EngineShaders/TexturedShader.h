#pragma once

#include "../../../Core/Math/Vector4D.h"

struct TexturedMeshData
{
    Vector4D meshColor;
    Vector4D rm_uvScale;
    uint32 diffuseMapIdx;
    uint32 normalMapIdx;
    uint32 armMapIdx;
};
