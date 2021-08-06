#ifndef COMMONDEFINES_INCLUDE
#define COMMONDEFINES_INCLUDE

#define M_PI 3.1415926535897932384626433832795
#define M_INV_PI 0.318309886183790671537767526745
#define M_PT_LUMEN2CAND 0.25 * M_INV_PI
#define M_LUMEN2CAND(apexAngle) ((0.5 * M_INV_PI )/(1 - apexAngle))
#define M_CM2M 0.01

#define MIN_N_DOT_V 0.01

#define SQR(v) ((v)*(v))

#define COLOR_2_UINT_BGRA(color) ((uint)((uint(color.a * 255) << 24) | (uint(color.r * 255) << 16) | (uint(color.g * 255) << 8)  | (uint(color.b * 255) << 0)))
#define UINT_BGRA_2_COLOR(packed) (vec4(((packed >> 16) & 0xFF) / 255.0f, ((packed >> 8) & 0xFF) / 255.0f, ((packed >> 0) & 0xFF) / 255.0f, ((packed >> 24) & 0xFF) / 255.0f))
#define COLOR_2_UINT_RGBA(color) ((uint)((uint(color.a * 255) << 24) | (uint(color.b * 255) << 16) | (uint(color.g * 255) << 8)  | (uint(color.r * 255) << 0)))
#define UINT_RGBA_2_COLOR(packed) (vec4(((packed >> 0) & 0xFF) / 255.0f, ((packed >> 8) & 0xFF) / 255.0f, ((packed >> 16) & 0xFF) / 255.0f, ((packed >> 24) & 0xFF) / 255.0f))

#define ENGINE_WORLD_TO_CUBE_DIR(worldDir) ((worldDir).yzx)

#endif // COMMONDEFINES_INCLUDE