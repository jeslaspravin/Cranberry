/*!
 * \file Texture2D.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "TexturesBase.h"
#include "Types/Colors.h"

struct Texture2DCreateParams : public TextureBaseCreateParams
{
    Size2D textureSize;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 0;
    // Must be Size == textureSize.x * textureSize.y
    std::vector<Color> colorData;
    // whether colorData is encoded in sRGB or needs to be stored in sRGB
    bool bIsSrgb = false;
    bool bIsNormalMap = false;
    // Useful incase of non normal textures
    uint8 componentsCount = 4;
    // Color that will be used if any pixel data is not available in color collection
    Color defaultColor = ColorConst::BLACK;
};

// Texture 2Ds are texture that will be static and gets created from certain data
class Texture2D
    : public TextureBase
    , public IRenderMemoryResource
{
public:
    std::vector<class Color> rawData;

public:
    uint32 getMipCount() const;
    void setData(const std::vector<class Color> &newData, const Color &defaultColor);
    bool isSrgb() const;

    /* IRenderMemoryResource overrides */
    ReferenceCountPtr<MemoryResource> renderResource() const final { return { textureResource }; }
    /* Overrides end */

    static Texture2D *createTexture(const Texture2DCreateParams &createParams);
    static void destroyTexture(Texture2D *texture2D);

protected:
    Texture2D() = default;
    ~Texture2D() = default;

    void reinitResources() override;

private:
    static void init(Texture2D *texture, bool bIsNormalMap, bool bIsSrgb, uint8 componentCount);
    static void destroy(Texture2D *texture);
    static EPixelDataFormat::Type determineDataFormat(
        bool bIsSrgb, bool bIsNormalMap, uint8 componentCount);
};

struct Texture2DRWCreateParams : public TextureBaseCreateParams
{
    Size2D textureSize;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 0;
    // Must be Size == textureSize.x * textureSize.y
    std::vector<Color> colorData;
    // Color that will be used if any pixel data is not available in color collection
    Color defaultColor = ColorConst::BLACK;

    EPixelDataFormat::Type format;
    bool bIsWriteOnly = false;
};

/*
 * Read write non atomic texture
 */
class Texture2DRW
    : public TextureBase
    , public IRenderMemoryResource
{
private:
    bool bIsWriteOnly;

public:
    std::vector<class Color> rawData;

public:
    uint32 getMipCount() const;
    void setData(const std::vector<class Color> &newData, const Color &defaultColor, bool bIsSrgb);

    /* IRenderMemoryResource overrides */
    ReferenceCountPtr<MemoryResource> renderResource() const final { return { textureResource }; }
    /* Overrides end */

    static Texture2DRW *createTexture(const Texture2DRWCreateParams &createParams);
    static void destroyTexture(Texture2DRW *texture2D);

protected:
    Texture2DRW() = default;
    ~Texture2DRW() = default;

    void reinitResources() override;

private:
    static void init(Texture2DRW *texture);
    static void destroy(Texture2DRW *texture);
};
