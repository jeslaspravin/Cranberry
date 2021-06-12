#pragma once
#include "../Platform/PlatformTypes.h"
#include "../Math/CoreMathTypedefs.h"

#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_float3.hpp>

class LinearColor;

class Color
{
private:
    Byte4D colorValue;
public:

    Color();
    explicit Color(Byte3D& value);
    explicit Color(Byte4D& value);
    // If values are in srgb bIsSrgb must be true
    explicit Color(uint8 r, uint8 g, uint8 b, uint8 a = 255, bool bIsSrgb = false);
    // if linear color has to be stored after converting to srgb then bAsSrgb must be true
    Color(const LinearColor& linearColor, bool bAsSrgb = false);
    Color(const Color& otherColor);
    Color(Color&& otherColor);
    void operator=(const Color& otherColor);
    void operator=(Color&& otherColor);

    inline Color toSrgb() const;
    inline Color toLinear() const;

    const Byte4D& getColorValue() const { return colorValue; }
    uint8 r() const { return colorValue.r; }
    void setR(uint8 r) { colorValue.r = r; }
    uint8 g() const { return colorValue.g; }
    void setG(uint8 g) { colorValue.g = g; }
    uint8 b() const { return colorValue.b; }
    void setB(uint8 b) { colorValue.b = b; }
    uint8 a() const { return colorValue.a; }
    void setA(uint8 a) { colorValue.a = a; }
    Byte3D rgb() const { return Byte3D(colorValue.r, colorValue.g, colorValue.b); }
};

class LinearColor
{
private:
    glm::vec4 colorValue;
public:
    LinearColor();
    explicit LinearColor(glm::vec3& value);
    explicit LinearColor(glm::vec4& value);
    explicit LinearColor(float r, float g, float b, float a = 1.0f);
    // If store the color value as it is bCheckSrgb must be false, if color has to be converted to linear then true
    LinearColor(const Color& color);
    LinearColor(const LinearColor& otherColor);
    LinearColor(LinearColor&& otherColor);
    void operator=(const LinearColor& otherColor);
    void operator=(LinearColor&& otherColor);

    const glm::vec4& getColorValue() const { return colorValue; }
    float r() const { return colorValue.r; }
    void setR(float r) { colorValue.r = r; }
    float g() const { return colorValue.g; }
    void setG(float g) { colorValue.g = g; }
    float b() const { return colorValue.b; }
    void setB(float b) { colorValue.b = b; }
    float a() const { return colorValue.a; }
    void setA(float a) { colorValue.a = a; }
    glm::vec3 rgb() const { return glm::vec3(colorValue.r, colorValue.g, colorValue.b); }
};

namespace ColorConst
{
    const Color WHITE(255, 255, 255, 255);
    const Color BLACK(0, 0, 0, 255);
}

namespace LinearColorConst
{
    const LinearColor WHITE(1, 1, 1, 1);
    const LinearColor BLACK(0, 0, 0, 1);
}