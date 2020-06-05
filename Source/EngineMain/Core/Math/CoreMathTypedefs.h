#pragma once

#include "../Platform/PlatformTypes.h"

#include <glm/detail/type_vec1.hpp>
#include <glm/detail/type_vec2.hpp>
#include <glm/detail/type_vec3.hpp>
#include <glm/detail/type_vec4.hpp>

#undef PI
#define PI (3.1415926535897932f)
#define SMALL_EPSILON (1.e-8f)
#define SLIGHTLY_SMALL_EPSILON (1.e-4f)

#ifndef FLT_EPSILON
#define FLT_EPSILON (1.192092896e-07f)
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

typedef glm::vec<3, uint32, glm::defaultp> Size3D;
typedef glm::vec<2, uint32, glm::defaultp> Size2D;

typedef glm::vec<3, uint8, glm::defaultp> Byte3D;
typedef glm::vec<4, uint8, glm::defaultp> Byte4D;

typedef glm::vec<2, float, glm::defaultp> Matrix2Col;
typedef glm::vec<3, float, glm::defaultp> Matrix3Col;
typedef glm::vec<4, float, glm::defaultp> Matrix4Col;
