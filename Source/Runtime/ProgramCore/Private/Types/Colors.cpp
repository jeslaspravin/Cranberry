/*!
 * \file Colors.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Colors.h"
#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"

/* Color implementations */

Color::Color()
    : colorValue(0, 0, 0, 0)
{}

Color::Color(Byte3D &value) { colorValue = Byte4D(value, 255); }

Color::Color(Byte4D &value)
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

Color::Color(const LinearColor &linearColor, bool bAsSrgb /*= false*/)
{
    if (bAsSrgb)
    {
        colorValue = Color(linearColor).toSrgb().colorValue;
    }
    else
    {
        colorValue = Byte4D(
            Math::clamp<uint8>(uint8(Math::round(linearColor.r() * 255)), 0u, 255u),
            Math::clamp<uint8>(uint8(Math::round(linearColor.g() * 255)), 0u, 255u),
            Math::clamp<uint8>(uint8(Math::round(linearColor.b() * 255)), 0u, 255u),
            Math::clamp<uint8>(uint8(Math::round(linearColor.a() * 255)), 0u, 255u)
        );
    }
}

Color::Color(const Color &otherColor)
    : colorValue(otherColor.colorValue)
{}

Color::Color(Color &&otherColor)
    : colorValue(std::move(otherColor.colorValue))
{}

void Color::operator=(Color &&otherColor) { colorValue = otherColor.colorValue; }

void Color::operator=(const Color &otherColor) { colorValue = std::move(otherColor.colorValue); }

/*
 * sRGB to Linear and Vice versa referred from
 * https://www.nayuki.io/page/srgb-transform-library  and
 * https://entropymine.com/imageworsener/srgbformula/
 */
Color Color::toSrgb() const
{
    glm::vec3 value = NORMALIZE_COLOR_COMP(glm::vec3(colorValue));

    return Color(
        uint8(Math::round(255 * (value.r > 0.0031308f ? (1.055f * Math::pow(value.r, 1 / 2.4f)) - 0.055f : value.r * 12.92f))),
        uint8(Math::round(255 * (value.g > 0.0031308f ? (1.055f * Math::pow(value.g, 1 / 2.4f)) - 0.055f : value.g * 12.92f))),
        uint8(Math::round(255 * (value.b > 0.0031308f ? (1.055f * Math::pow(value.b, 1 / 2.4f)) - 0.055f : value.b * 12.92f))), colorValue.a
    );
}

Color Color::toLinear() const
{
    glm::vec3 value = NORMALIZE_COLOR_COMP(glm::vec3(colorValue));

    return Color(
        uint8(Math::round(255 * (value.r > 0.04045f ? Math::pow((value.r + 0.055f) / 1.055f, 2.4f) : value.r / 12.92f))),
        uint8(Math::round(255 * (value.g > 0.04045f ? Math::pow((value.g + 0.055f) / 1.055f, 2.4f) : value.g / 12.92f))),
        uint8(Math::round(255 * (value.b > 0.04045f ? Math::pow((value.b + 0.055f) / 1.055f, 2.4f) : value.b / 12.92f))), colorValue.a
    );
}

uint8 Color::operator[](uint32 idx) const { return colorValue[idx]; }

Color::operator uint32() const { return *reinterpret_cast<const uint32 *>(this); }

Color Color::fromHsl(const Vector3D &hsl, uint8 alpha /*= 255*/) { return Color(LinearColor::fromHsl(hsl), NORMALIZE_COLOR_COMP(alpha)); }

LinearColor Color::fromHsv(const Vector3D &hsv, uint8 alpha /*= 255*/) { return Color(LinearColor::fromHsv(hsv), NORMALIZE_COLOR_COMP(alpha)); }

Vector3D Color::toHsv() const { return LinearColor(*this).toHsv(); }

Vector3D Color::toHsl() const { return LinearColor(*this).toHsl(); }

/* Linear Color implementations */

LinearColor::LinearColor()
    : colorValue(0, 0, 0, 0)
{}

LinearColor::LinearColor(glm::vec3 &value)
    : colorValue(value, 1.0f)
{}

LinearColor::LinearColor(glm::vec4 &value)
    : colorValue(value)
{}

LinearColor::LinearColor(float r, float g, float b, float a /*= 1.0f*/)
    : colorValue(r, g, b, a)
{}

LinearColor::LinearColor(const Color &color)
    : colorValue(NORMALIZE_COLOR_COMP(glm::vec4(color.getColorValue())))
{}

LinearColor::LinearColor(const LinearColor &otherColor)
    : colorValue(otherColor.colorValue)
{}

LinearColor::LinearColor(LinearColor &&otherColor)
    : colorValue(std::move(otherColor.colorValue))
{}

LinearColor::LinearColor(const Vector4D &value)
    : colorValue(value.x(), value.y(), value.z(), value.w())
{}

float LinearColor::operator[](uint32 idx) const { return colorValue[idx]; }

// http://en.wikipedia.org/wiki/HSL_color_space
Vector3D LinearColor::toHsl() const
{
    Vector3D hsl;
    float &h = hsl[0];
    float &s = hsl[1];
    float &l = hsl[2];

    float max = Math::max(colorValue.r, colorValue.g, colorValue.b);
    float min = Math::min(colorValue.r, colorValue.g, colorValue.b);
    h = s = l = (max + min) / 2;

    if (max == min) // Chroma == 0
    {
        h = s = 0; // achromatic
    }
    else
    {
        float chroma = max - min;

        // if L > 0.5? (C/1 - 2L + 1) : (C/2L)
        s = (l > 0.5f) ? chroma / (2 - max - min) : chroma / (max + min);

        if (max == colorValue.r)
        {
            h = Math::mod((colorValue.g - colorValue.b) / chroma + 6, 6);
        }
        else if (max == colorValue.g)
        {
            h = (colorValue.b - colorValue.r) / chroma + 2;
        }
        else if (max == colorValue.b)
        {
            h = (colorValue.r - colorValue.g) / chroma + 4;
        }
        // h * 60deg / 360deg to normalize between 0 to 1
        h /= 6;
    }

    return hsl;
}

// http://en.wikipedia.org/wiki/HSL_color_space
Vector3D LinearColor::toHsv() const
{
    Vector3D hsv;
    float &h = hsv[0];
    float &s = hsv[1];
    float &v = hsv[2];

    float max = Math::max(colorValue.r, colorValue.g, colorValue.b);
    float min = Math::min(colorValue.r, colorValue.g, colorValue.b);

    v = max;
    if (max == min) // Chroma == 0
    {
        h = s = 0; // achromatic
    }
    else
    {
        float chroma = max - min;

        s = (max == 0.0f) ? 0.0f : chroma / max;

        if (max == colorValue.r)
        {
            h = Math::mod((colorValue.g - colorValue.b) / chroma + 6, 6);
        }
        else if (max == colorValue.g)
        {
            h = (colorValue.b - colorValue.r) / chroma + 2;
        }
        else if (max == colorValue.b)
        {
            h = (colorValue.r - colorValue.g) / chroma + 4;
        }
        // h * 60deg / 360deg to normalize between 0 to 1
        h /= 6;
    }
    return hsv;
}

void hsxToRGBSwizzled(LinearColor &color, int32 swizzleIdx, const float *toRGB)
{
    static const int32 rgbSwizzle[6][3] = {
        {2, 0, 3},
        {1, 2, 3},
        {3, 2, 0},
        {3, 1, 2},
        {0, 3, 2},
        {2, 3, 1}
    };

    color.setR(toRGB[rgbSwizzle[swizzleIdx][0]]);
    color.setG(toRGB[rgbSwizzle[swizzleIdx][1]]);
    color.setB(toRGB[rgbSwizzle[swizzleIdx][2]]);
}

#if 0
// http://en.wikipedia.org/wiki/HSL_color_space
float hue2rgb(float p, float q, float t)
{
    if (t < 0)
    {
        t += 1;
    }
    if (t > 1)
    {
        t -= 1;
    }
    if (t < (1.0f / 6.0f))
    {
        return p + (q - p) * 6 * t;
    }
    if (t < 0.5f)
    {
        return q;
    }
    if (t < (2.0f / 3.0f))
    {
        return p + (q - p) * ((2.0f / 3.0f) - t) * 6;
    }
    return p;
}

// http://en.wikipedia.org/wiki/HSL_color_space
LinearColor LinearColor::fromHsl(const Vector3D& hsl)
{
    const float& h = hsl[0];
    const float& s = hsl[1];
    const float& l = hsl[2];
    LinearColor retColor;
    retColor.setA(1);
    if (s == 0.0f)
    {
        // monochromatic
        retColor.colorValue.r = retColor.colorValue.g = retColor.colorValue.b = l;
    }
    else 
    {
        float q = (l < 0.5f) ? l * (1 + s) : (l + s - l * s);
        float p = 2 * l - q;
        retColor.colorValue.r = hue2rgb(p, q, h + 1.0f / 3.0f);
        retColor.colorValue.g = hue2rgb(p, q, h);
        retColor.colorValue.b = hue2rgb(p, q, h - 1.0f / 3.0f);
    }
    return retColor;
}

#else
// http://en.wikipedia.org/wiki/HSL_color_space
LinearColor LinearColor::fromHsl(const Vector3D &hsl, float alpha /*= 1.0f*/)
{
    const float &h = hsl[0];
    const float &s = hsl[1];
    const float &l = hsl[2];
    LinearColor retColor;
    retColor.setA(alpha);
    if (s == 0.0f)
    {
        // monochromatic
        retColor.colorValue.r = retColor.colorValue.g = retColor.colorValue.b = l;
    }
    else
    {
        const float hx6 = h * 6;
        const float hx6Frac = Math::frac(hx6);
        const int32 hx6floor = int32(Math::floor(hx6));
        float toRGB[4];
        if (l > 0.5f)
        {
            toRGB[0] = l * (1 + s * (1 - 2 * hx6Frac)) - s * (1 - 2 * hx6Frac); // m + x (even hx6)
            toRGB[1] = l * (1 - s * (1 - 2 * hx6Frac)) + s * (1 - 2 * hx6Frac); // m + x (odd hx6)
            toRGB[2] = l * (1 - s) + s;                                         // m + c
            toRGB[3] = l * (1 + s) - s;                                         // m
        }
        else
        {
            toRGB[0] = l * (1 - s * (1 - 2 * hx6Frac)); // m + x (even hx6)
            toRGB[1] = l * (1 - s * (2 * hx6Frac - 1)); // m + x (odd hx6)
            toRGB[2] = l * (1 + s);                     // m + c
            toRGB[3] = l * (1 - s);                     // m
        }

        hsxToRGBSwizzled(retColor, hx6floor, toRGB);
    }
    return retColor;
}
#endif

// http://en.wikipedia.org/wiki/HSL_color_space
LinearColor LinearColor::fromHsv(const Vector3D &hsv, float alpha /*= 1.0f*/)
{
    const float &h = hsv[0];
    const float &s = hsv[1];
    const float &v = hsv[2];
    LinearColor retColor;
    retColor.setA(alpha);
    if (s == 0.0f)
    {
        // monochromatic
        retColor.colorValue.r = retColor.colorValue.g = retColor.colorValue.b = v;
    }
    else
    {
        const float hx6 = h * 6;
        const float hx6Frac = Math::frac(hx6);
        const int32 hx6floor = int32(Math::floor(hx6));
        float toRGB[4]{
            v * (1 - s * (1 - hx6Frac)), // m + x (even hx6)
            v * (1 - s * hx6Frac),       // m + x (odd hx6)
            v,                           // m + c
            v * (1 - s)                  // m
        };

        hsxToRGBSwizzled(retColor, hx6floor, toRGB);
    }
    return retColor;
}

void LinearColor::operator=(LinearColor &&otherColor) { colorValue = otherColor.colorValue; }

void LinearColor::operator=(const LinearColor &otherColor) { colorValue = std::move(otherColor.colorValue); }

namespace ColorConst
{
Color random(uint8 alpha /*= 255*/) { return Color(LinearColorConst::random(NORMALIZE_COLOR_COMP(alpha))); }

const Color WHITE_Transparent(255, 255, 255, 0);
const Color BLACK_Transparent(0, 0, 0, 0);
const Color WHITE(255, 255, 255, 255);
const Color GRAY(128, 128, 128, 255);
const Color BLACK(0, 0, 0, 255);
const Color RED(255, 0, 0, 255);
const Color BLUE(0, 0, 255, 255);
const Color PALE_BLUE(195, 218, 234, 255);
const Color GREEN(0, 255, 0, 255);

const Color CYAN(0, 255, 255, 255);
} // namespace ColorConst

namespace LinearColorConst
{
LinearColor random(float alpha /*= 1.0f*/)
{
    // use golden ratio
    float goldenRatioConjugate = 1 / GOLDEN_RATIO;

    float h = Math::random();
    h += goldenRatioConjugate;
    h = Math::frac(h);
    // return LinearColor::fromHsl(Vector3D(h, 0.5f, 0.5f));
    return LinearColor::fromHsv(Vector3D(h, 0.5f, 1.0f), alpha);
}

const LinearColor WHITE_Transparent(1, 1, 1, 0);
const LinearColor BLACK_Transparent(0, 0, 0, 0);
const LinearColor WHITE(1, 1, 1, 1);
const LinearColor GRAY(0.5, 0.5, 0.5, 1);
const LinearColor BLACK(0, 0, 0, 1);
const LinearColor RED(1, 0, 0, 1);
const LinearColor BLUE(0, 0, 1, 1);
const LinearColor PALE_BLUE(0.764f, 0.854f, 0.917f, 1.0f);
const LinearColor GREEN(0, 1, 0, 1);

const LinearColor CYAN(0, 1, 1, 1);
} // namespace LinearColorConst
