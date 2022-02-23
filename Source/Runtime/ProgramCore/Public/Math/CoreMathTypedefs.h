/*!
 * \file CoreMathTypedefs.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"

#include <glm/detail/type_vec1.hpp>
#include <glm/detail/type_vec2.hpp>
#include <glm/detail/type_vec3.hpp>
#include <glm/detail/type_vec4.hpp>

#undef PI
#define PI (3.1415926535897932f)
#define GOLDEN_RATIO (1.618033f)
#define SMALL_EPSILON (1.e-6f)
#define SLIGHTLY_SMALL_EPSILON (1.e-4f)

#ifndef FLT_EPSILON
#define FLT_EPSILON (1.192092896e-7f)
#endif
#ifndef FLT_MIN
#define FLT_MIN (1.175494351e-38f)
#endif
#ifndef FLT_MAX
#define FLT_MAX (3.402823466e+38f)
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-016
#endif
#ifndef DBL_MAX
#define DBL_MAX 1.7976931348623158e+308
#endif
#ifndef DBL_MIN
#define DBL_MIN 2.2250738585072014e-308
#endif

typedef glm::vec<4, uint32, glm::defaultp> Size4D;
typedef glm::vec<3, uint32, glm::defaultp> Size3D;
typedef glm::vec<2, uint32, glm::defaultp> Size2D;
typedef glm::vec<2, uint16, glm::defaultp> ShortSize2D;

typedef glm::vec<4, int32, glm::defaultp> Int4D;
typedef glm::vec<3, int32, glm::defaultp> Int3D;
typedef glm::vec<2, int32, glm::defaultp> Int2D;
typedef glm::vec<2, int16, glm::defaultp> Short2D;

typedef glm::vec<3, uint8, glm::defaultp> Byte2D;
typedef glm::vec<3, uint8, glm::defaultp> Byte3D;
typedef glm::vec<4, uint8, glm::defaultp> Byte4D;

typedef glm::vec<2, float, glm::defaultp> Matrix2Col;
typedef glm::vec<3, float, glm::defaultp> Matrix3Col;
typedef glm::vec<4, float, glm::defaultp> Matrix4Col;
