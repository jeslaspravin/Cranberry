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
    bool bSrgb;
public:

    Color();
    Color(Byte3D& value);
    Color(Byte4D& value);
    Color(uint8 r, uint8 g, uint8 b, uint8 a = 255);
    Color(const LinearColor& linearColor, bool bAsSrgb = false);
    Color(const Color& otherColor);
    Color(Color&& otherColor);

    inline Color toSrgb() const;
    inline Color toLinear() const;

    const Byte4D& getColorValue() const { return colorValue; }
    uint8 r() const { return colorValue.r; }
    uint8 g() const { return colorValue.g; }
    uint8 b() const { return colorValue.b; }
    uint8 a() const { return colorValue.a; }
    Byte3D rgb() const { return Byte3D(colorValue.r, colorValue.g, colorValue.b); }
};

class LinearColor
{
private:
    glm::vec4 colorValue;
public:
    LinearColor();
    LinearColor(glm::vec3& value);
    LinearColor(glm::vec4& value);
    LinearColor(float r, float g, float b, float a = 1.0f);
    LinearColor(const Color& color, bool bFromSrgb = false);
    LinearColor(const LinearColor& otherColor);
    LinearColor(LinearColor&& otherColor);

    const glm::vec4& getColorValue() { return colorValue; }
    float r() const { return colorValue.r; }
    float g() const { return colorValue.g; }
    float b() const { return colorValue.b; }
    float a() const { return colorValue.a; }
    glm::vec3 rgb() const { return glm::vec3(colorValue.r, colorValue.g, colorValue.b); }
};