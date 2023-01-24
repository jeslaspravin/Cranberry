/*!
 * \file Colors.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypes.h"
#include "Types/CoreTypes.h"

class LinearColor;

#define NORMALIZE_COLOR_COMP(Val) ((Val) / 255.0f)

class PROGRAMCORE_EXPORT Color
{
private:
    Byte4D colorValue;

public:
    Color();
    explicit Color(Byte3D &value);
    explicit Color(Byte4D &value);
    // If values are in srgb bIsSrgb must be true
    explicit Color(uint8 r, uint8 g, uint8 b, uint8 a = 255, bool bIsSrgb = false);
    // if linear color has to be stored after converting to srgb then bAsSrgb must be true
    Color(const LinearColor &linearColor, bool bAsSrgb = false);
    Color(const Color &otherColor);
    Color(Color &&otherColor);
    void operator=(const Color &otherColor);
    void operator=(Color &&otherColor);

    Color toSrgb() const;
    Color toLinear() const;
    Vector3D toHsl() const;
    Vector3D toHsv() const;

    const Byte4D &getColorValue() const { return colorValue; }
    Byte4D &getColorValue() { return colorValue; }
    uint8 r() const { return colorValue.r; }
    void setR(uint8 r) { colorValue.r = r; }
    uint8 g() const { return colorValue.g; }
    void setG(uint8 g) { colorValue.g = g; }
    uint8 b() const { return colorValue.b; }
    void setB(uint8 b) { colorValue.b = b; }
    uint8 a() const { return colorValue.a; }
    void setA(uint8 a) { colorValue.a = a; }
    Byte3D rgb() const { return Byte3D(colorValue.r, colorValue.g, colorValue.b); }

    uint8 operator[](uint32 idx) const;
    // As RGBA packed
    FORCE_INLINE operator uint32() const { return *reinterpret_cast<const uint32 *>(this); }
    // As RGBA packed, MSB <- 0xAAGGBBRR -> LSB
    FORCE_INLINE uint32 rgba() const { return uint32(*this); }
    // As BGRA packed, MSB <- 0xAARRBBBB -> LSB
    FORCE_INLINE uint32 bgra() const { return (uint32(a()) << 24) | (uint32(r()) << 16) | (uint32(g()) << 8) | b(); }

    static Color fromHsl(const Vector3D &hsl, uint8 alpha = 255);
    static LinearColor fromHsv(const Vector3D &hsv, uint8 alpha = 255);
};

class PROGRAMCORE_EXPORT LinearColor
{
private:
    glm::vec4 colorValue;

public:
    LinearColor();
    explicit LinearColor(glm::vec3 &value);
    explicit LinearColor(glm::vec4 &value);
    explicit LinearColor(const Vector4D &value);
    LinearColor(float r, float g, float b, float a = 1.0f);
    // If store the color value as it is bCheckSrgb must be false, if color has to be converted to linear
    // then true
    LinearColor(const Color &color);
    LinearColor(const LinearColor &otherColor);
    LinearColor(LinearColor &&otherColor);
    void operator=(const LinearColor &otherColor);
    void operator=(LinearColor &&otherColor);

    Vector3D toHsl() const;
    Vector3D toHsv() const;

    const glm::vec4 &getColorValue() const { return colorValue; }
    glm::vec4 &getColorValue() { return colorValue; }
    operator Vector4D() const { return Vector4D(colorValue); }
    float r() const { return colorValue.r; }
    void setR(float r) { colorValue.r = r; }
    float g() const { return colorValue.g; }
    void setG(float g) { colorValue.g = g; }
    float b() const { return colorValue.b; }
    void setB(float b) { colorValue.b = b; }
    float a() const { return colorValue.a; }
    void setA(float a) { colorValue.a = a; }
    glm::vec3 rgb() const { return glm::vec3(colorValue.r, colorValue.g, colorValue.b); }

    float operator[](uint32 idx) const;
    static LinearColor fromHsl(const Vector3D &hsl, float alpha = 1.0f);
    static LinearColor fromHsv(const Vector3D &hsv, float alpha = 1.0f);
};

namespace ColorConst
{
PROGRAMCORE_EXPORT Color random(uint8 alpha = 255);

// TODO(Jeslas) : Make these constants const expressionable
extern PROGRAMCORE_EXPORT const Color WHITE_Transparent;
extern PROGRAMCORE_EXPORT const Color BLACK_Transparent;
extern PROGRAMCORE_EXPORT const Color WHITE;
extern PROGRAMCORE_EXPORT const Color GRAY;
extern PROGRAMCORE_EXPORT const Color BLACK;
extern PROGRAMCORE_EXPORT const Color YELLOW;
extern PROGRAMCORE_EXPORT const Color RED;
extern PROGRAMCORE_EXPORT const Color GREEN;
extern PROGRAMCORE_EXPORT const Color BLUE;

extern PROGRAMCORE_EXPORT const Color DARK_GRAY;
extern PROGRAMCORE_EXPORT const Color PALE_BLUE;

extern PROGRAMCORE_EXPORT const Color CYAN;
} // namespace ColorConst

namespace LinearColorConst
{
PROGRAMCORE_EXPORT LinearColor random(float alpha = 1.0f);

extern PROGRAMCORE_EXPORT const LinearColor WHITE_Transparent;
extern PROGRAMCORE_EXPORT const LinearColor BLACK_Transparent;
extern PROGRAMCORE_EXPORT const LinearColor WHITE;
extern PROGRAMCORE_EXPORT const LinearColor GRAY;
extern PROGRAMCORE_EXPORT const LinearColor BLACK;
extern PROGRAMCORE_EXPORT const LinearColor YELLOW;
extern PROGRAMCORE_EXPORT const LinearColor RED;
extern PROGRAMCORE_EXPORT const LinearColor GREEN;
extern PROGRAMCORE_EXPORT const LinearColor BLUE;

extern PROGRAMCORE_EXPORT const LinearColor DARK_GRAY;
extern PROGRAMCORE_EXPORT const LinearColor PALE_BLUE;

extern PROGRAMCORE_EXPORT const LinearColor CYAN;
} // namespace LinearColorConst