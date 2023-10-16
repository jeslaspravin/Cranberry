/*!
 * \file CoreMathTypedefs.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"
#include "Types/CompilerDefines.h"

// Math - GLM
#ifndef GLM_HEADER_INCLUDES_BEGIN
#define GLM_HEADER_INCLUDES_BEGIN                                                                                                              \
    COMPILER_PRAGMA(COMPILER_PUSH_WARNING)                                                                                                     \
    COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_UNINITIALIZED))
#endif
#ifndef GLM_HEADER_INCLUDES_END
#define GLM_HEADER_INCLUDES_END COMPILER_PRAGMA(COMPILER_POP_WARNING)
#endif

GLM_HEADER_INCLUDES_BEGIN

#include <glm/detail/type_vec1.hpp>
#include <glm/detail/type_vec2.hpp>
#include <glm/detail/type_vec3.hpp>
#include <glm/detail/type_vec4.hpp>

GLM_HEADER_INCLUDES_BEGIN

#undef PI
#define PI (3.1415926535897932f)
#define GOLDEN_RATIO (1.618033f)
#define SMALL_EPSILON (1.e-8f)
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

typedef glm::vec<4, uint32, glm::defaultp> UInt4;
typedef glm::vec<3, uint32, glm::defaultp> UInt3;
typedef glm::vec<2, uint32, glm::defaultp> UInt2;
typedef glm::vec<2, uint16, glm::defaultp> UShort2;

typedef glm::vec<4, int32, glm::defaultp> Int4;
typedef glm::vec<3, int32, glm::defaultp> Int3;
typedef glm::vec<2, int32, glm::defaultp> Int2;
typedef glm::vec<2, int16, glm::defaultp> Short2;

typedef glm::vec<3, uint8, glm::defaultp> Byte2;
typedef glm::vec<3, uint8, glm::defaultp> Byte3;
typedef glm::vec<4, uint8, glm::defaultp> Byte4;

typedef glm::vec<2, float, glm::defaultp> Matrix2Col;
typedef glm::vec<3, float, glm::defaultp> Matrix3Col;
typedef glm::vec<4, float, glm::defaultp> Matrix4Col;
