#include "Colors.h"

#include <glm/common.hpp>
#include <glm/exponential.hpp>

/* Color implementations */

Color::Color()
    : colorValue(0,0,0,0)
    , bSrgb(false)
{}

Color::Color(Byte3D& value)
    : bSrgb(false)
{
    colorValue = Byte4D(value,255);
}

Color::Color(Byte4D& value)
    : colorValue(value)
    , bSrgb(false)
{}

Color::Color(uint8 r, uint8 g, uint8 b, uint8 a /*= 255*/)
    : bSrgb(false)
{
    colorValue = Byte4D(r, g, b, a);
}

Color::Color(const LinearColor& linearColor, bool bAsSrgb /*= false*/)
    : bSrgb(bAsSrgb)
{
    if (bAsSrgb)
    {
        colorValue = Color(linearColor).toSrgb().colorValue;
    }
    else
    {
        colorValue = Byte4D(
            glm::clamp<uint8>(uint8(linearColor.r() * 255), 0u, 255u),
            glm::clamp<uint8>(uint8(linearColor.g() * 255), 0u, 255u),
            glm::clamp<uint8>(uint8(linearColor.b() * 255), 0u, 255u),
            glm::clamp<uint8>(uint8(linearColor.a() * 255), 0u, 255u)
        );
    }
}

Color::Color(const Color& otherColor)
    : colorValue(otherColor.colorValue)
    , bSrgb(otherColor.bSrgb)
{}

Color::Color(Color&& otherColor)
    : colorValue(std::move(otherColor.colorValue))
    , bSrgb(std::move(otherColor.bSrgb))
{}

/*
* sRGB to Linear and Vice versa refered from 
* https://www.nayuki.io/page/srgb-transform-library  and  
* https://entropymine.com/imageworsener/srgbformula/
*/
Color Color::toSrgb() const
{
    if (bSrgb)
    {
        return *this;
    }

    glm::vec3 value = glm::vec3(colorValue) / 255.f;

    return Color(
        uint8(255 * (value.r > 0.0031308f ? (1.055f * glm::pow(value.r, 1 / 2.4f)) - 0.055f : value.r * 12.92f)),
        uint8(255 * (value.g > 0.0031308f ? (1.055f * glm::pow(value.g, 1 / 2.4f)) - 0.055f : value.g * 12.92f)),
        uint8(255 * (value.b > 0.0031308f ? (1.055f * glm::pow(value.b, 1 / 2.4f)) - 0.055f : value.b * 12.92f)),
        colorValue.a
    );
}

Color Color::toLinear() const
{
    if (!bSrgb)
    {
        return *this;
    }

    glm::vec3 value = glm::vec3(colorValue) / 255.f;

    return Color(
        uint8(255 * (value.r > 0.04045f ? glm::pow((value.r + 0.055f) / 1.055f, 2.4f) : value.r / 12.92f)),
        uint8(255 * (value.g > 0.04045f ? glm::pow((value.g + 0.055f) / 1.055f, 2.4f) : value.g / 12.92f)),
        uint8(255 * (value.b > 0.04045f ? glm::pow((value.b + 0.055f) / 1.055f, 2.4f) : value.b / 12.92f)),
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

LinearColor::LinearColor(const Color& color, bool bFromSrgb /*= false*/)
{
    if (bFromSrgb)
    {
        colorValue = glm::vec4(color.toLinear().getColorValue()) / 255.f;
    }
    else
    {
        colorValue = glm::vec4(color.getColorValue()) / 255.f;
    }
}

LinearColor::LinearColor(const LinearColor& otherColor)
    :colorValue(otherColor.colorValue)
{}

LinearColor::LinearColor(LinearColor&& otherColor)
    :colorValue(std::move(otherColor.colorValue))
{}
