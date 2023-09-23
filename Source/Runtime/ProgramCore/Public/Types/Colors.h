/*!
 * \file Colors.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypes.h"
#include "Math/Math.h"
#include "Types/CoreTypes.h"

class LinearColor;

#define NORMALIZE_COLOR_COMP(Val) ((Val) / 255.0f)

class PROGRAMCORE_EXPORT Color
{
private:
    union
    {
        Byte4 colorValue;
        uint32 rgbaValue;
    };

public:
    constexpr Color()
        : colorValue(0, 0, 0, 0)
    {}
    constexpr explicit Color(Byte3 value) { colorValue = Byte4(value, 255); }
    constexpr explicit Color(Byte4 value)
        : colorValue(value)
    {}
    // If values are in srgb bIsSrgb must be true
    constexpr explicit Color(uint8 r, uint8 g, uint8 b, uint8 a = 255, bool bIsSrgb = false)
    {
        colorValue = Byte4(r, g, b, a);
        if (bIsSrgb)
        {
            colorValue = toLinear().colorValue;
        }
    }
    // Below separation as consteval is necessary to keep colorValue and (x, y, z, w) as the active union member
    constexpr explicit Color(uint32 inRGBA)
    {
        if (std::is_constant_evaluated())
        {
            colorValue = { (inRGBA & 0xFF), ((inRGBA >> 8) & 0xFF), ((inRGBA >> 16) & 0xFF), ((inRGBA >> 24) & 0xFF) };
        }
        else
        {
            rgbaValue = inRGBA;
        }
    }
    // if linear color has to be stored after converting to srgb then bAsSrgb must be true
    Color(const LinearColor &linearColor, bool bAsSrgb = false);
    MAKE_TYPE_DEFAULT_COPY_MOVE(Color)

    Color toSrgb() const;
    Color toLinear() const;

    FORCE_INLINE Vector3 toHsl() const;
    FORCE_INLINE Vector3 toHsv() const;

    FORCE_INLINE const Byte4 &getColorValue() const { return colorValue; }
    FORCE_INLINE Byte4 &getColorValue() { return colorValue; }
    constexpr uint8 r() const { return colorValue.x; }
    FORCE_INLINE void setR(uint8 r) { colorValue.x = r; }
    constexpr uint8 g() const { return colorValue.y; }
    FORCE_INLINE void setG(uint8 g) { colorValue.y = g; }
    constexpr uint8 b() const { return colorValue.z; }
    FORCE_INLINE void setB(uint8 b) { colorValue.z = b; }
    constexpr uint8 a() const { return colorValue.w; }
    FORCE_INLINE void setA(uint8 a) { colorValue.w = a; }
    constexpr Byte3 rgb() const { return Byte3(colorValue.x, colorValue.y, colorValue.z); }

    constexpr uint8 operator[] (uint32 idx) const { return colorValue[idx]; }

    // As RGBA packed, MSB <- 0xAAGGBBRR -> LSB
    constexpr uint32 rgba() const
    {
        if (std::is_constant_evaluated())
        {
            return (uint32(a()) << 24) | (uint32(b()) << 16) | (uint32(g()) << 8) | r();
        }
        else
        {
            return rgbaValue;
        }
    }
    // As BGRA packed, MSB <- 0xAARRBBBB -> LSB
    constexpr uint32 bgra() const { return (uint32(a()) << 24) | (uint32(r()) << 16) | (uint32(g()) << 8) | b(); }
    // As RGBA packed
    constexpr operator uint32 () const { return rgba(); }

    FORCE_INLINE static Color fromHsl(const Vector3 &hsl, uint8 alpha = 255);
    FORCE_INLINE static LinearColor fromHsv(const Vector3 &hsv, uint8 alpha = 255);
};

class PROGRAMCORE_EXPORT LinearColor
{
private:
    glm::vec4 colorValue;

public:
    LinearColor();
    explicit LinearColor(glm::vec3 &value);
    explicit LinearColor(glm::vec4 &value);
    explicit LinearColor(const Vector4 &value);
    LinearColor(float r, float g, float b, float a = 1.0f);
    LinearColor(Color color)
        : colorValue(NORMALIZE_COLOR_COMP(glm::vec4(color.getColorValue())))
    {}
    MAKE_TYPE_DEFAULT_COPY_MOVE(LinearColor)

    Vector3 toHsl() const;
    Vector3 toHsv() const;

    const glm::vec4 &getColorValue() const { return colorValue; }
    glm::vec4 &getColorValue() { return colorValue; }
    operator Vector4 () const { return Vector4(colorValue); }
    float r() const { return colorValue.r; }
    void setR(float r) { colorValue.r = r; }
    float g() const { return colorValue.g; }
    void setG(float g) { colorValue.g = g; }
    float b() const { return colorValue.b; }
    void setB(float b) { colorValue.b = b; }
    float a() const { return colorValue.a; }
    void setA(float a) { colorValue.a = a; }
    glm::vec3 rgb() const { return glm::vec3(colorValue.r, colorValue.g, colorValue.b); }

    float operator[] (uint32 idx) const;
    static LinearColor fromHsl(const Vector3 &hsl, float alpha = 1.0f);
    static LinearColor fromHsv(const Vector3 &hsv, float alpha = 1.0f);
};

FORCE_INLINE Color Color::fromHsl(const Vector3 &hsl, uint8 alpha /*= 255*/)
{
    return Color(LinearColor::fromHsl(hsl), NORMALIZE_COLOR_COMP(alpha));
}
FORCE_INLINE LinearColor Color::fromHsv(const Vector3 &hsv, uint8 alpha /*= 255*/)
{
    return Color(LinearColor::fromHsv(hsv), NORMALIZE_COLOR_COMP(alpha));
}

FORCE_INLINE Vector3 Color::toHsv() const { return LinearColor(*this).toHsv(); }
FORCE_INLINE Vector3 Color::toHsl() const { return LinearColor(*this).toHsl(); }

#include "Types/ColorConstants.inl"