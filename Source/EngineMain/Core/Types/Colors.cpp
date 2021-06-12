#include "Colors.h"
#include "../Math/Math.h"

/* Color implementations */

Color::Color()
    : colorValue(0,0,0,0)
{}

Color::Color(Byte3D& value)
{
    colorValue = Byte4D(value,255);
}

Color::Color(Byte4D& value)
    : colorValue(value)
{}

Color::Color(uint8 r, uint8 g, uint8 b, uint8 a /*= 255*/, bool bIsSrgb /*= false */)
{
    colorValue = Byte4D(r, g, b, a);
    if (bIsSrgb)
    {
        colorValue = toLinear().colorValue;
    }
}

Color::Color(const LinearColor& linearColor, bool bAsSrgb /*= false*/)
{
    if (bAsSrgb)
    {
        colorValue = Color(linearColor).toSrgb().colorValue;
    }
    else
    {
        colorValue = Byte4D(
            Math::clamp<uint8>(uint8(linearColor.r() * 255), 0u, 255u),
            Math::clamp<uint8>(uint8(linearColor.g() * 255), 0u, 255u),
            Math::clamp<uint8>(uint8(linearColor.b() * 255), 0u, 255u),
            Math::clamp<uint8>(uint8(linearColor.a() * 255), 0u, 255u)
        );
    }
}

Color::Color(const Color& otherColor)
    : colorValue(otherColor.colorValue)
{}

Color::Color(Color&& otherColor)
    : colorValue(std::move(otherColor.colorValue))
{}

void Color::operator=(Color && otherColor)
{
    colorValue = otherColor.colorValue;
}

void Color::operator=(const Color & otherColor)
{
    colorValue = std::move(otherColor.colorValue);
}

/*
* sRGB to Linear and Vice versa refered from 
* https://www.nayuki.io/page/srgb-transform-library  and  
* https://entropymine.com/imageworsener/srgbformula/
*/
Color Color::toSrgb() const
{
    glm::vec3 value = glm::vec3(colorValue) / 255.f;

    return Color(
        uint8(255 * (value.r > 0.0031308f ? (1.055f * Math::pow(value.r, 1 / 2.4f)) - 0.055f : value.r * 12.92f)),
        uint8(255 * (value.g > 0.0031308f ? (1.055f * Math::pow(value.g, 1 / 2.4f)) - 0.055f : value.g * 12.92f)),
        uint8(255 * (value.b > 0.0031308f ? (1.055f * Math::pow(value.b, 1 / 2.4f)) - 0.055f : value.b * 12.92f)),
        colorValue.a
    );
}

Color Color::toLinear() const
{
    glm::vec3 value = glm::vec3(colorValue) / 255.f;

    return Color(
        uint8(255 * (value.r > 0.04045f ? Math::pow((value.r + 0.055f) / 1.055f, 2.4f) : value.r / 12.92f)),
        uint8(255 * (value.g > 0.04045f ? Math::pow((value.g + 0.055f) / 1.055f, 2.4f) : value.g / 12.92f)),
        uint8(255 * (value.b > 0.04045f ? Math::pow((value.b + 0.055f) / 1.055f, 2.4f) : value.b / 12.92f)),
        colorValue.a
    );
}

/* Linear Color implementations */

LinearColor::LinearColor()
    : colorValue(0, 0, 0, 0)
{}

LinearColor::LinearColor(glm::vec3& value)
    : colorValue(value, 1.0f)
{}

LinearColor::LinearColor(glm::vec4& value)
    : colorValue(value)
{}

LinearColor::LinearColor(float r, float g, float b, float a /*= 1.0f*/)
    : colorValue(r,g,b,a)
{}

LinearColor::LinearColor(const Color& color)
    : colorValue(glm::vec4(color.getColorValue()) / 255.f)
{}

LinearColor::LinearColor(const LinearColor& otherColor)
    :colorValue(otherColor.colorValue)
{}

LinearColor::LinearColor(LinearColor&& otherColor)
    :colorValue(std::move(otherColor.colorValue))
{}

void LinearColor::operator=(LinearColor&& otherColor)
{
    colorValue = otherColor.colorValue;
}

void LinearColor::operator=(const LinearColor& otherColor)
{
    colorValue = std::move(otherColor.colorValue);
}
