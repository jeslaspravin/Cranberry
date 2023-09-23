/*!
 * \file FontManager.cpp
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "FontManager.h"
#include "Math/Box.h"
#include "Math/CoreMathTypes.h"
#include "Math/Math.h"
#include "Math/MathGeom.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Resources/MemoryResources.h"

#include <array>
#include <unordered_set>

// Have all function as static
#define STBTT_STATIC

#define STBTT_assert(x) debugAssert(x)
// Math defines
#define STBTT_ifloor(x) ((int32)Math::floor(x))
#define STBTT_iceil(x) ((int32)Math::ceil(x))
#define STBTT_sqrt(x) Math::sqrt(x)
#define STBTT_pow(x, y) Math::pow(x, y)
#define STBTT_fmod(x, y) Math::modf(x, y)
#define STBTT_cos(x) Math::cos(x)
#define STBTT_acos(x) Math::acos(x)
#define STBTT_fabs(x) Math::abs(x)

#define STB_TRUETYPE_IMPLEMENTATION

COMPILER_PRAGMA(COMPILER_PUSH_WARNING)
COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_UNNEEDED_INTERNAL_FUNCTION))

#include "stb_truetype.h"

COMPILER_PRAGMA(COMPILER_POP_WARNING)

CONST_EXPR static const TChar TAB_CHAR = TCHAR('\t');
CONST_EXPR static const TChar SPACE_CHAR = TCHAR(' ');
CONST_EXPR static const TChar NEWLINE_CHAR = TCHAR('\n');
CONST_EXPR static const TChar CRETURN_CHAR = TCHAR('\r'); // Will be skipped
CONST_EXPR static const TChar QUESTION_CHAR = TCHAR('?');
CONST_EXPR static const uint32 UNKNOWN_GLYPH = 0xFFFD;

// From https://www.compart.com/en/unicode/category/Zs
CONST_EXPR static const uint32 UNICODE_SPACES[] = { SPACE_CHAR, TAB_CHAR, NEWLINE_CHAR, 0x00A0 /*No Break space*/,
                                                    0x1680,     0x2000,   0x2001,       0x2002,
                                                    0x2003,     0x2004,   0x2005,       0x2006,
                                                    0x2007,     0x2008,   0x2009,       0x200A,
                                                    0x202F,     0x205F,   0x3000 };

CONST_EXPR static const int32 TAB_SIZE = 4;
CONST_EXPR static const uint16 ATLAS_MAX_SIZE = 2048;
CONST_EXPR static const uint16 BORDER_SIZE = 1;

class FontManagerContext
{
private:
    const FontManager *owner;

public:
    // 21bits(11-31) Unicode point, 6bits(5-10) Font index, 5bits(0-4) Height of font in FontHeight
    // multiplier(Stores 0 to 31 values representing 0 -> 16, 1 x 32, 2 x 32,... 31 x 32)
    using GlyphIndex = uint32;
    using FontHeight = uint8;
    using FontIndex = FontManager::FontIndex;

    struct FontInfo
    {
        stbtt_fontinfo stbFont;
        std::vector<uint8> fontData;
        String fontName;
        // Number of pixels above baseline this font extends(Unscaled)
        int32 ascent;
        // Number of pixels below baseline this font drops(Unscaled)
        int32 descent;
        // Number of pixels to new baseline from current(Unscaled)
        int32 newLine;
        // Fall back glyph that will always be present
        uint32 fallbackCode = UNKNOWN_GLYPH;
        // Add additional font specific informations here
    };

    // A Glyph(Character) in a font
    struct FontGlyph
    {
        // Bitmap data start index in cached bitmap data
        int64 bitmapDataIdx = -1;
        // Index of a glyph in a font sheet
        int32 glyphIdx = 0;
        // Pixels to add to arrive at next character start for this glyph(Scaled)
        int32 advance = 0;
        // Pixels to add to offset the glyph from current horizontal point(Scaled)
        int32 lsb = 0;
        // Number of pixels above baseline this glyph extends(Scaled)
        int32 ascent = 0;
        // Number of pixels below baseline this glyph drops(Scaled)
        int32 descent = 0;
        // Index to texture atlas
        int32 texCoordIdx = -1;
        uint8 texAtlasIdx = 0;
    };
    // This struct is created such that GlyphCoord can be casted from textCoords after packing
    struct GlyphCoords
    {
        // Texture coordinate in texture atlas, In texels including border
        UShortRect texCoords;
        GlyphIndex contextGlyphIdx;
    };

    FontIndex defaultFont;
    std::vector<FontInfo> allFonts;
    std::unordered_map<GlyphIndex, FontGlyph> allGlyphs;
    std::vector<GlyphCoords> allGlyphCoords;
    // We support maximum 2 atlas, now
    ImageResourceRef textureAtlases[2];
    UInt2 atlasSizes[ARRAY_LENGTH(textureAtlases)];
    std::vector<uint8> bitmapCache;

    std::unordered_set<GlyphIndex> glyphsPending;

private:
    DEBUG_INLINE uint32 findFallbackCodepoint(FontIndex font) noexcept;

public:
    FontManagerContext(const FontManager *inOwner) noexcept
        : owner(inOwner)
        , defaultFont(0){};

    FORCE_INLINE static UShortRect clipBorder(const UShortRect &inTexCoord) noexcept
    {
        return UShortRect(inTexCoord.minBound + BORDER_SIZE, inTexCoord.maxBound - BORDER_SIZE);
    }

    FORCE_INLINE static FontHeight pixelsToHeight(uint32 heightInPixels) noexcept
    {
        uint32 contextHeight = heightInPixels / 32u;
        // Only if height in pixels is above 16 then we have to ceil the value
        contextHeight += (heightInPixels > 16u) && (heightInPixels % 32u > 0);
        return FontHeight(Math::min(contextHeight, 31u));
    }

    FORCE_INLINE static uint32 heightToPixels(FontHeight height) noexcept { return Math::max(height * 32u, 16u); }

    FORCE_INLINE static void fromGlyphIndex(uint32 &outCodepoint, FontIndex &outFontIndex, FontHeight &outHeight, GlyphIndex glyph) noexcept
    {
        // Mask first 5bits
        outHeight = (glyph & 0x1Fu);
        // Right shift by 5 and mask next 6bits for font index
        glyph /= 32u;
        outFontIndex = (glyph & 0x3Fu);
        // Right shift by 6
        outCodepoint = (glyph / 64u);
    }

    // FontManagerContext::pixelsToHeight to get FontHeight for pixel height
    FORCE_INLINE static GlyphIndex toGlyphIndex(uint32 codepoint, FontIndex fontIndex, FontHeight height) noexcept
    {
        GlyphIndex retVal = 0;
        retVal = codepoint;
        // Shift left by 6bits and add fontIndex
        retVal = (retVal * 64u) + (fontIndex & 0x3Fu);
        // Shift left by 5bits and add height
        retVal = (retVal * 32u) + (height & 0x1Fu);
        return retVal;
    }

    FORCE_INLINE FontIndex addFont(const std::vector<uint8> &fontData, const String &fontName) noexcept
    {
        FontIndex idx = FontIndex(allFonts.size());

        FontInfo &fontInfo = allFonts.emplace_back();
        fontInfo.fontData = fontData;
        fontInfo.fontName = fontName;

        int32 fontInitialized
            = stbtt_InitFont(&fontInfo.stbFont, fontInfo.fontData.data(), stbtt_GetFontOffsetForIndex(fontInfo.fontData.data(), 0));
        debugAssert(fontInitialized > 0);

        stbtt_GetFontVMetrics(&fontInfo.stbFont, &fontInfo.ascent, &fontInfo.descent, &fontInfo.newLine);
        fontInfo.newLine += (fontInfo.ascent - fontInfo.descent);

        fontInfo.fallbackCode = findFallbackCodepoint(idx);

        return idx;
    }

    // Wrapper
    // Codepoint's equivalent Glyph index in font
    // Use only if this data is not available in FontGlyph data
    FORCE_INLINE uint32 codepointToFontGlyphIndex(FontIndex font, uint32 codepoint) const noexcept
    {
        return stbtt_FindGlyphIndex(&allFonts[font].stbFont, codepoint);
    }

    // Wrapper
    // Scale factor to make font of given size from font's size
    FORCE_INLINE float scaleToPixelHeight(FontIndex font, uint32 heightInPixels) const noexcept
    {
        return stbtt_ScaleForPixelHeight(&allFonts[font].stbFont, float(heightInPixels));
    }
    // Scale factor to make glyph height to requested height in pixels
    FORCE_INLINE static float scaleHeightToPixelHeight(uint32 heightInPixels, FontHeight height) noexcept
    {
        return float(heightInPixels) / float(heightToPixels(height));
    }

    // Wrapper
    FORCE_INLINE void glyphHMetrics(FontIndex font, const FontGlyph &glyph, int32 &advance, int32 &leftSideBearing) const noexcept
    {
        stbtt_GetGlyphHMetrics(&allFonts[font].stbFont, glyph.glyphIdx, &advance, &leftSideBearing);
    }

    // Wrapper
    // Bounding box in texture space. (x0, y0) top left, (x1, y1) bottom right
    // Box is scaled with provided scaling
    FORCE_INLINE void glyphBitmapBoxSubPixel(
        FontIndex font, const FontGlyph &glyph, float scale, float xShift, float yShift, int32 &x0, int32 &y0, int32 &x1, int32 &y1
    ) const noexcept
    {
        stbtt_GetGlyphBitmapBoxSubpixel(&allFonts[font].stbFont, glyph.glyphIdx, scale, scale, xShift, yShift, &x0, &y0, &x1, &y1);
    }

    // Wrapper
    // Fills the bitmap in outBitmap for this glyph and uses bitmapStride to move to next row
    // glyphWidth and glyphHeight are used as scissor and viewport size for font rasterizer
    // Bitmap is scaled with provided scaling
    FORCE_INLINE void glyphBitmapSubPixel(
        FontIndex font, const FontGlyph &glyph, float scale, float xShift, float yShift, uint8 *outBitmap, int32 glyphWidth, int32 glyphHeight,
        int32 bitmapStride
    ) const noexcept
    {
        // stbtt_MakeGlyphBitmapSubpixel(
        //     &allFonts[font].stbFont, outBitmap, glyphWidth, glyphHeight, bitmapStride, scale, scale, xShift, yShift, glyph.glyphIdx
        //);

        float subX, subY;
        stbtt_MakeGlyphBitmapSubpixelPrefilter(
            &allFonts[font].stbFont, outBitmap, glyphWidth, glyphHeight, bitmapStride, scale, scale, xShift, yShift, 2, 2, &subX, &subY,
            glyph.glyphIdx
        );
    }

    // Wrapper
    // Kern advance if next character is glyph2
    // Advance value is unscaled
    FORCE_INLINE int32 glyphKernAdvance(FontIndex font, const FontGlyph &glyph1, const FontGlyph &glyph2) const noexcept
    {
        return stbtt_GetGlyphKernAdvance(&allFonts[font].stbFont, glyph1.glyphIdx, glyph2.glyphIdx);
    }

    // Utility functions

    // Just finds glyph and does not adds incoming glyph
    FORCE_INLINE const FontGlyph *findGlyph(uint32 codepoint, FontIndex font, FontHeight height) const noexcept
    {
        auto glyphItr = allGlyphs.find(toGlyphIndex(codepoint, font, height));
        if (glyphItr == allGlyphs.cend())
        {
            glyphItr = allGlyphs.find(toGlyphIndex(allFonts[font].fallbackCode, font, height));
        }
        return ((glyphItr != allGlyphs.cend()) ? &glyphItr->second : nullptr);
    }

    // Adds some necessary glyphs for this fonts at given height
    FORCE_INLINE void addNecessaryGlyphs(FontIndex font, FontHeight height) noexcept
    {
        CONST_EXPR static const uint32 NECESSARY_CODEPOINTS[] = { SPACE_CHAR, UNKNOWN_GLYPH, QUESTION_CHAR };
        for (uint32 codePt : NECESSARY_CODEPOINTS)
        {
            GlyphIndex contextGlyphIdx = toGlyphIndex(codePt, font, height);
            if (!allGlyphs.contains(contextGlyphIdx) && codepointToFontGlyphIndex(font, codePt))
            {
                glyphsPending.insert(contextGlyphIdx);
            }
        }
    }

    void updatePendingGlyphs() noexcept;

    bool isSpaceCode(uint32 codepoint) noexcept
    {
        for (uint32 i = 0; i < ARRAY_LENGTH(UNICODE_SPACES); ++i)
        {
            if (UNICODE_SPACES[i] == codepoint)
            {
                return true;
            }
        }
        return false;
    }
    // gives advance value for given spaces and return true if one of the handled spaces
    // xAdvance is given out in glyph scaled value
    // yAdvance is scaled to fontToHeightScale value
    FORCE_INLINE bool advanceSpace(
        uint32 codepoint, FontIndex font, const FontGlyph &spaceGlyph, float fontToHeightScale, int32 &xAdvance, int32 &yAdvance
    ) noexcept
    {
        xAdvance = 0;
        yAdvance = 0;
        switch (codepoint)
        {
        case SPACE_CHAR:
        {
            xAdvance = spaceGlyph.advance;
            break;
        }
        case TAB_CHAR:
        {
            xAdvance = TAB_SIZE * spaceGlyph.advance;
            break;
        }
        case NEWLINE_CHAR:
        {
            yAdvance = int32(fontToHeightScale * allFonts[font].newLine);
            break;
        }
        case CRETURN_CHAR:
        {
            break;
        }
        default:
        {
            alertAlwaysf(!isSpaceCode(codepoint), "Unhandled space {}", codepoint);
            return false;
        }
        };
        return true;
    }
};

DEBUG_INLINE uint32 FontManagerContext::findFallbackCodepoint(FontIndex font) noexcept
{
    CONST_EXPR static const uint32 FALLBACK_CHARS[] = { UNKNOWN_GLYPH, QUESTION_CHAR, SPACE_CHAR };
    for (uint32 codePt : FALLBACK_CHARS)
    {
        if (codepointToFontGlyphIndex(font, codePt))
        {
            return codePt;
        }
    }
    fatalAssertf(false, "No fall-back code point found for font at {}", font);
    return UNKNOWN_GLYPH;
}

void FontManagerContext::updatePendingGlyphs() noexcept
{
    if (glyphsPending.empty())
    {
        return;
    }
    CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("UpdatePendingGlyphs"));

    allGlyphCoords.reserve(allGlyphCoords.size() + glyphsPending.size());
    allGlyphs.reserve(allGlyphs.size() + glyphsPending.size());
    for (const GlyphIndex &contextGlyphIdx : glyphsPending)
    {
        uint32 codepoint;
        FontIndex font;
        FontHeight height;
        fromGlyphIndex(codepoint, font, height, contextGlyphIdx);

        uint32 fontHeightPixels = heightToPixels(height);
        float fontToGlyphScale = scaleToPixelHeight(font, fontHeightPixels);

        uint32 glyphIdx = codepointToFontGlyphIndex(font, codepoint);
        if (glyphIdx == 0)
        {
            continue;
        }

        FontGlyph &glyph = allGlyphs[contextGlyphIdx];
        glyph.glyphIdx = glyphIdx;
        glyphHMetrics(font, glyph, glyph.advance, glyph.lsb);
        glyph.advance = int32(glyph.advance * fontToGlyphScale);
        glyph.lsb = int32(glyph.lsb * fontToGlyphScale);

        IRect bitmapBox;
        glyphBitmapBoxSubPixel(
            font, glyph, fontToGlyphScale, 0, 0, bitmapBox.minBound.x, bitmapBox.minBound.y, bitmapBox.maxBound.x, bitmapBox.maxBound.y
        );
        Int2 bitmapSize = bitmapBox.size();
        int32 texelsCount = bitmapSize.x * bitmapSize.y;
        // Will be 0 for space characters
        if (texelsCount != 0)
        {
            // Since min value is one ascending from baseline
            glyph.ascent = bitmapBox.minBound.y;
            glyph.descent = bitmapBox.maxBound.y;
            glyph.bitmapDataIdx = int64(bitmapCache.size());
            glyph.texCoordIdx = int32(allGlyphCoords.size());

            GlyphCoords &glyphCoords = allGlyphCoords.emplace_back();
            glyphCoords.contextGlyphIdx = contextGlyphIdx;
            // Add border texels to size
            glyphCoords.texCoords
                = UShortRect{ UShort2(0), UShort2(bitmapSize.x, bitmapSize.y) + UShortRect::PointElementType(2 * BORDER_SIZE) };

            bitmapCache.resize(bitmapCache.size() + texelsCount);
            glyphBitmapSubPixel(
                font, glyph, fontToGlyphScale, 0, 0, &bitmapCache[glyph.bitmapDataIdx], bitmapSize.x, bitmapSize.y, bitmapSize.x
            );
        }
    }
    glyphsPending.clear();

    std::vector<UShortRect *> packRects;
    packRects.reserve(allGlyphs.size());
    // Convert each of rectangles to be placed at origin
    for (std::pair<const GlyphIndex, FontGlyph> &glyphPair : allGlyphs)
    {
        if (glyphPair.second.texCoordIdx == -1)
        {
            continue;
        }
        GlyphCoords &glyphCoords = allGlyphCoords[glyphPair.second.texCoordIdx];
        UShortRect::PointType rectSize = glyphCoords.texCoords.size();
        glyphCoords.texCoords.minBound = UShortRect::PointType(0);
        glyphCoords.texCoords.maxBound = rectSize;

        packRects.emplace_back(&glyphCoords.texCoords);
    }

    std::vector<PackedRectsBin<UShortRect>> packedBins;
    std::vector<std::vector<Color>> atlasTexels;
    if (MathGeom::packRectangles(packedBins, UShortRect::PointType(ATLAS_MAX_SIZE), packRects))
    {
        alertAlwaysf(
            packedBins.size() <= ARRAY_LENGTH(textureAtlases),
            "Packing fonts in unsuccessful in {} texture atlases extend atlas count if necessary", ARRAY_LENGTH(textureAtlases)
        );
        static_assert(
            ARRAY_LENGTH(textureAtlases) <= std::numeric_limits<decltype(FontGlyph::texAtlasIdx)>::max(),
            "Texture atlas count is above max index that can be stored at FontGlyph::texAtlasIdx"
        );

        for (uint8 i = 0; i < ARRAY_LENGTH(textureAtlases) && i < packedBins.size(); ++i)
        {
            const UShort2 &atlasSize = packedBins[i].binSize;

            atlasSizes[i] = atlasSize;
            std::vector<Color> &atlasTexs = atlasTexels.emplace_back();
            atlasTexs.resize(atlasSize.x * atlasSize.y);
            for (UShortRect *glyphBox : packedBins[i].rects)
            {
                GlyphCoords &glyphCoords = *reinterpret_cast<GlyphCoords *>(glyphBox);
                FontGlyph &glyph = allGlyphs[glyphCoords.contextGlyphIdx];
                glyph.texAtlasIdx = i;

                // Offset border so we copy only to glyph
                UShortRect bound = clipBorder(glyphCoords.texCoords);
                UShort2 boundSize = bound.size();

                // Copy all rows in glyph from bitmap to pixels
                for (uint32 y = bound.minBound.y; y < bound.maxBound.y; ++y)
                {
                    uint32 yOffset = y - bound.minBound.y;
                    for (uint32 x = bound.minBound.x; x < bound.maxBound.x; ++x)
                    {
                        uint32 xOffset = x - bound.minBound.x;

                        // X columns constitutes a Row
                        uint32 texIdx = y * atlasSize.x + x;
                        // In Bitmap glyphs are stored as individual continuous
                        // stream so no need for additional stride
                        uint32 bitmapIdx = uint32(glyph.bitmapDataIdx + (yOffset * boundSize.x) + xOffset);

                        uint8 bitmap = bitmapCache[bitmapIdx];
                        atlasTexs[texIdx] = Color(bitmap, bitmap, bitmap, bitmap);
                    }
                }
            }
        }
    }
    else
    {
        fatalAssertf(false, "Packing fonts failed");
        return;
    }

    owner->broadcastPreTextureAtlasUpdate();
    ENQUEUE_RENDER_COMMAND(UpdateFontGlyphs)
    (
        [this, atlasTexels](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            CBE_PROFILER_SCOPE(CBE_PROFILER_CHAR("UploadGlyphAtlas"));

            for (int32 i = 0; i < atlasTexels.size(); ++i)
            {
                ImageResourceCreateInfo ci{
                    .imageFormat = EPixelDataFormat::R_U8_Norm, .dimensions = {atlasSizes[i].x, atlasSizes[i].y, 1},
                         .numOfMips = 1
                };
                textureAtlases[i] = graphicsHelper->createImage(graphicsInstance, ci);
                textureAtlases[i]->setShaderUsage(EImageShaderUsage::Sampling);
                textureAtlases[i]->setResourceName("FontAtlas_" + String::toString(i));
                textureAtlases[i]->init();

                cmdList->copyToImage(textureAtlases[i], atlasTexels[i]);
            }
            owner->broadcastTextureAtlasUpdated();
        }
    );
}

//////////////////////////////////////////////////////////////////////////
/// FontManager Implementations
//////////////////////////////////////////////////////////////////////////

FontManager::FontManager(EInitType) { context = new FontManagerContext(this); }

FontManager::FontManager(FontManager &&otherManager)
{
    context = new (otherManager.context) FontManagerContext(this);
    otherManager.context = nullptr;
}

FontManager &FontManager::operator= (FontManager &&otherManager)
{
    context = new (otherManager.context) FontManagerContext(this);
    otherManager.context = nullptr;
    return (*this);
}

FontManager::~FontManager() { clear(); }

void FontManager::clear()
{
    if (context)
    {
        delete context;
        context = nullptr;
    }
}

FontManager::FontIndex FontManager::addFont(const String &fontPath) const
{
    PlatformFile fontFile{ fontPath };
    fontFile.setFileFlags(EFileFlags::Read);
    fontFile.setCreationAction(EFileFlags::OpenExisting);
    fontFile.setSharingMode(EFileSharing::ReadOnly);
    fatalAssertf(fontFile.exists(), "Font file {} not found", fontPath);

    fontFile.openFile();
    std::vector<uint8> fontData;
    fontFile.read(fontData);
    fontFile.closeFile();

    FontIndex fontIdx = context->addFont(fontData, PathFunctions::stripExtension(fontFile.getFileName()));
    onFontAdded.invoke(fontIdx);
    return fontIdx;
}

FontManager::FontIndex FontManager::addFont(const std::vector<uint8> &fontData, const String &fontName) const
{
    FontIndex fontIdx = context->addFont(fontData, fontName);
    onFontAdded.invoke(fontIdx);
    return fontIdx;
}

void FontManager::addGlyphsFromStr(const String &str, FontIndex font, uint32 height) const
{
    uint32 lowestCodePoint = std::numeric_limits<uint32>::max();
    uint32 highestCodePoint = 0;
    for (uint32 codepoint : StringCodePoints(str))
    {
        if (codepoint < 128u && std::isspace(TChar(codepoint)))
        {
            continue;
        }
        lowestCodePoint = Math::min(lowestCodePoint, codepoint);
        highestCodePoint = Math::max(highestCodePoint, codepoint);
    }

    addGlyphs(font, { lowestCodePoint, highestCodePoint }, height);
}

void FontManager::addGlyphs(FontIndex font, const std::vector<ValueRange<uint32>> &glyphCodeRanges, const std::vector<uint32> &heights) const
{
    for (uint32 height : heights)
    {
        FontManagerContext::FontHeight contextHeight = FontManagerContext::pixelsToHeight(height);

        context->addNecessaryGlyphs(font, contextHeight);
        for (const ValueRange<uint32> &glyphCodeRange : glyphCodeRanges)
        {
            for (uint32 codePt = glyphCodeRange.minBound; codePt < glyphCodeRange.maxBound; ++codePt)
            {
                FontManagerContext::GlyphIndex glyphIdx = FontManagerContext::toGlyphIndex(codePt, font, contextHeight);
                // If not duplicate and valid glyph
                if (context->codepointToFontGlyphIndex(font, codePt) && !context->allGlyphs.contains(glyphIdx))
                {
                    context->glyphsPending.insert(glyphIdx);
                }
            }
        }
    }
}

void FontManager::addGlyphs(FontIndex font, const ValueRange<uint32> &glyphCodeRange, uint32 height) const
{
    FontManagerContext::FontHeight contextHeight = FontManagerContext::pixelsToHeight(height);

    context->addNecessaryGlyphs(font, contextHeight);
    for (uint32 codePt = glyphCodeRange.minBound; codePt < glyphCodeRange.maxBound; ++codePt)
    {
        FontManagerContext::GlyphIndex glyphIdx = FontManagerContext::toGlyphIndex(codePt, font, contextHeight);
        // If not duplicate and valid glyph
        if (context->codepointToFontGlyphIndex(font, codePt) && !context->allGlyphs.contains(glyphIdx))
        {
            context->glyphsPending.insert(glyphIdx);
        }
    }
}

void FontManager::flushUpdates() const { context->updatePendingGlyphs(); }

void FontManager::setupTextureAtlas(ShaderParameters *shaderParams, const String &paramName)
{
    ENQUEUE_RENDER_COMMAND(SetupTextureAtlas)
    (
        [this, shaderParams, paramName](
            class IRenderCommandList * /*cmdList*/, IGraphicsInstance * /*graphicsInstance*/, const GraphicsHelperAPI * /*graphicsHelper*/
        )
        {
            for (int32 i = 0; i < ARRAY_LENGTH(context->textureAtlases); ++i)
            {
                shaderParams->setTextureParam(paramName.getChar(), context->textureAtlases[i], i);
                static ImageViewInfo fontTextureView = {
                    .componentMapping = {.g = EPixelComponentMapping::R, .b = EPixelComponentMapping::R, .a = EPixelComponentMapping::R}
                };
                shaderParams->setTextureParamViewInfo(paramName.getChar(), fontTextureView);
            }
        }
    );
}

uint32 FontManager::calculateRenderWidth(const String &text, FontIndex font, uint32 height) const
{
    if (text.empty() || (context->allFonts.size() <= font))
    {
        return 0u;
    }
    context->updatePendingGlyphs();

    FontManagerContext::FontHeight contextHeight = FontManagerContext::pixelsToHeight(height);
    // Glyph will be scaled already and below value can be used to scale glyph scaled values to height
    // scaled value
    float glyphToHeightScale = FontManagerContext::scaleHeightToPixelHeight(height, contextHeight);
    // For font related unscaled value to height scaled value
    float fontToHeightScale = context->scaleToPixelHeight(font, height);
    // For scaling font unscaled to glyph scaled
    float fontToGlyphScale = context->scaleToPixelHeight(font, FontManagerContext::heightToPixels(contextHeight));

    const FontManagerContext::FontGlyph *spaceGlyph = context->findGlyph(SPACE_CHAR, font, contextHeight);
    alertAlwaysf(spaceGlyph, "Invalid space glyph! Add glyphs to fontmanager for font, height combination");

    // Max width in case there is line feed character
    int32 width = 0, maxWidth = 0;
    const FontManagerContext::FontGlyph *lastGlyph = nullptr;
    for (uint32 codepoint : StringCodePoints(text))
    {
        int32 xAdvance = 0, yAdvance = 0;
        // Just get code point as that is enough to determine spaces
        if (spaceGlyph && context->advanceSpace(codepoint, font, *spaceGlyph, fontToHeightScale, xAdvance, yAdvance))
        {
            width += xAdvance;
            if (yAdvance != 0)
            {
                // If we advance y it means new line, So set max width and zero current line
                // width
                maxWidth = Math::max(maxWidth, width);
                width = 0;
            }
            lastGlyph = nullptr;
            continue;
        }

        const FontManagerContext::FontGlyph *codeGlyph = context->findGlyph(codepoint, font, contextHeight);
        if (codeGlyph)
        {
            if (lastGlyph)
            {
                width += int32(fontToGlyphScale * context->glyphKernAdvance(font, *lastGlyph, *codeGlyph));
            }

            width += codeGlyph->advance;
            lastGlyph = codeGlyph;
        }
    }
    // Last line
    maxWidth = Math::max(maxWidth, width);

    // Will not be less than 0 since max width starts at 0
    return uint32(Math::ceil(maxWidth * glyphToHeightScale));
}

uint32 FontManager::calculateRenderHeight(const String &text, FontIndex font, uint32 height, int32 wrapWidth /*= -1*/) const
{
    if (context->allFonts.size() <= font)
    {
        return 0u;
    }
    context->updatePendingGlyphs();

    FontManagerContext::FontHeight contextHeight = FontManagerContext::pixelsToHeight(height);
    // For font related unscaled value to height scaled value
    float fontToHeightScale = context->scaleToPixelHeight(font, height);
    // Glyph will be scaled already and below value can be used to scale glyph scaled values to height
    // scaled value
    float glyphToHeightScale = FontManagerContext::scaleHeightToPixelHeight(height, contextHeight);

    const FontManagerContext::FontGlyph *spaceGlyph = context->findGlyph(SPACE_CHAR, font, contextHeight);
    alertAlwaysf(spaceGlyph, "Invalid space glyph! Add glyphs to fontmanager for font, height combination");

    // Always return at least 1 line worth of height
    int32 outHeight = (context->allFonts[font].ascent - context->allFonts[font].descent);
    // If empty text just returning single line size
    if (text.empty())
    {
        return uint32(fontToHeightScale * outHeight);
    }

    // Last word width to add to lineWidth, all are height scaled
    int32 lineWidth = 0, lastWordWidth = 0;
    const FontManagerContext::FontGlyph *lastGlyph = nullptr;
    for (uint32 codepoint : StringCodePoints(text))
    {
        int32 xAdvance = 0, yAdvance = 0;
        if (spaceGlyph && context->advanceSpace(codepoint, font, *spaceGlyph, 1u, xAdvance, yAdvance))
        {
            if (yAdvance != 0)
            {
                // If we advance y it means new line, So set line width to 0 and add to
                // height
                lineWidth = 0;
                outHeight += context->allFonts[font].newLine;
            }
            // If wrap width is small or we do not have any word to wrap then skip wrapping
            else if (lineWidth > 0 && lastWordWidth > 0 && wrapWidth >= 0 && (lineWidth + lastWordWidth) > wrapWidth)
            {
                lineWidth = lastWordWidth + int32(glyphToHeightScale * xAdvance);
                outHeight += context->allFonts[font].newLine;
            }
            // No wrapping done just add last word and space to current line width
            else
            {
                lineWidth += lastWordWidth + int32(glyphToHeightScale * xAdvance);
            }
            lastWordWidth = 0;
            lastGlyph = nullptr;
            continue;
        }

        const FontManagerContext::FontGlyph *codeGlyph = context->findGlyph(codepoint, font, contextHeight);
        if (codeGlyph)
        {
            if (lastGlyph)
            {
                lastWordWidth += int32(fontToHeightScale * context->glyphKernAdvance(font, *lastGlyph, *codeGlyph));
            }

            lastWordWidth += int32(glyphToHeightScale * codeGlyph->advance);
            lastGlyph = codeGlyph;
        }
    }

    return uint32(fontToHeightScale * outHeight);
}

void FontManager::draw(
    std::vector<FontVertex> &outVertices, IRect &outBB, const String &text, FontIndex font, uint32 height, int32 wrapWidth /*= -1*/
) const
{
    if (context->allFonts.size() <= font)
    {
        return;
    }
    context->updatePendingGlyphs();

    FontManagerContext::FontHeight contextHeight = FontManagerContext::pixelsToHeight(height);
    // For font related unscaled value to height scaled value
    float fontToHeightScale = context->scaleToPixelHeight(font, height);
    // Glyph will be scaled already and below value can be used to scale glyph scaled values to height
    // scaled value
    float glyphToHeightScale = FontManagerContext::scaleHeightToPixelHeight(height, contextHeight);

    const FontManagerContext::FontGlyph *spaceGlyph = context->findGlyph(SPACE_CHAR, font, contextHeight);
    alertAlwaysf(spaceGlyph, "Invalid space glyph! Add glyphs to fontmanager for font, height combination");

    outBB.reset(
        IRect::PointType{ std::numeric_limits<IRect::PointElementType>::max() },
        IRect::PointType{ std::numeric_limits<IRect::PointElementType>::min() }
    );

    // Pixels to shift for each new line
    int32 newLineH = int32(fontToHeightScale * context->allFonts[font].newLine);
    // Y offset from 0, Where this line will be rendered
    int32 baseline = 0;
    // X offset in each line, Where next letter will be rendered
    int32 cursorPos = 0;
    // Last glyph for kerning
    const FontManagerContext::FontGlyph *lastGlyph = nullptr;
    // last word's start vertex idx used for wrapping to new line, last word lsb is left side bearing
    // after last word is shifted to new line Last word width for auto wrapping decision
    int32 lastWordVertex = -1, lastWordLsb = 0, lastWordWidth = 0;
    auto wrapLastWord = [&newLineH, &lastWordVertex, &lastWordLsb](std::vector<FontVertex> &vertices, int32 &outCursorPos)
    {
        if (lastWordVertex >= 0 && lastWordVertex < vertices.size())
        {
            outCursorPos = lastWordLsb;
            int32 oldCursorPos = vertices[lastWordVertex].pos.x;
            for (uint32 i = lastWordVertex; i < vertices.size(); i += 4)
            {
                int32 width = vertices[i + 1].pos.x - vertices[i + 0].pos.x;
                // Add Current letter's start offset from last letter's old end along X to cursor
                // so we get this letter's new start
                outCursorPos += (vertices[i + 0].pos.x - oldCursorPos);
                oldCursorPos = vertices[i + 1].pos.x;

                // Update left edge 0 to 3
                vertices[i + 0].pos.x = outCursorPos;
                vertices[i + 0].pos.y += newLineH;
                vertices[i + 3].pos.x = outCursorPos;
                vertices[i + 3].pos.y += newLineH;
                // Offset cursor to right edge
                outCursorPos += width;
                // Update right edge 1 to 2
                vertices[i + 1].pos.x = outCursorPos;
                vertices[i + 1].pos.y += newLineH;
                vertices[i + 2].pos.x = outCursorPos;
                vertices[i + 2].pos.y += newLineH;
            }
        }
    };
    for (uint32 codepoint : StringCodePoints(text))
    {
        int32 xAdvance = 0, yAdvance = 0;
        if (spaceGlyph && context->advanceSpace(codepoint, font, *spaceGlyph, 1u, xAdvance, yAdvance))
        {
            // If wrap width is small or we do not have any word to wrap then skip wrapping
            if (cursorPos > 0 && lastWordVertex >= 0 && wrapWidth >= 0 && (cursorPos + lastWordWidth) > wrapWidth)
            {
                cursorPos = 0;
                wrapLastWord(outVertices, cursorPos);
                // New cursor pos after wrapping will have word width added to it
                // Account for this space alone
                cursorPos += int32(glyphToHeightScale * xAdvance);
                baseline += newLineH;
            }
            // No auto wrapping is done just add last word and space to current line width
            else
            {
                cursorPos += lastWordWidth + int32(glyphToHeightScale * xAdvance);
            }

            // If this space is a new line then add another new line after auto wrapping/cursor
            // shift
            if (yAdvance != 0)
            {
                // If we advance y it means new line, So set line width to 0 and add to
                // height
                cursorPos = 0;
                baseline += newLineH;
            }
            lastWordVertex = -1;
            lastWordWidth = 0;
            lastWordLsb = 0;
            lastGlyph = nullptr;
            continue;
        }

        const FontManagerContext::FontGlyph *codeGlyph = context->findGlyph(codepoint, font, contextHeight);
        if (codeGlyph)
        {
            debugAssert(codeGlyph->texCoordIdx >= 0);

            if (lastWordVertex < 0)
            {
                lastWordVertex = int32(outVertices.size());
                lastWordLsb = int32(glyphToHeightScale * codeGlyph->lsb);
                lastWordWidth = 0;
            }
            // Kerning must be done before adding this glyph's vertices
            if (lastGlyph)
            {
                lastWordWidth += int32(fontToHeightScale * context->glyphKernAdvance(font, *lastGlyph, *codeGlyph));
            }
            // Add vertices
            // Glyph related caches
            UShortRect glyphTexCoordClipped = context->clipBorder(context->allGlyphCoords[codeGlyph->texCoordIdx].texCoords);
            const auto &texSize = context->atlasSizes[codeGlyph->texAtlasIdx];

            // Width of this glyph's quad for given height scale
            int32 glyphLeft = cursorPos + lastWordWidth + int32(glyphToHeightScale * codeGlyph->lsb);
            int32 glyphRight = glyphLeft + int32(glyphTexCoordClipped.size().x * glyphToHeightScale);
            int32 glyphTop = baseline + int32(glyphToHeightScale * codeGlyph->ascent);
            int32 glyphBottom = baseline + int32(glyphToHeightScale * codeGlyph->descent);
            Rect texCoord{
                {glyphTexCoordClipped.minBound.x / float(texSize.x), glyphTexCoordClipped.minBound.y / float(texSize.y)},
                {glyphTexCoordClipped.maxBound.x / float(texSize.x), glyphTexCoordClipped.maxBound.y / float(texSize.y)}
            };

            uint32 glyphVert = uint32(outVertices.size());
            outVertices.resize(outVertices.size() + 4);
            // Add left edge 0 to 3
            outVertices[glyphVert + 0].atlasIdx = codeGlyph->texAtlasIdx;
            outVertices[glyphVert + 0].pos = { glyphLeft, glyphTop };
            outVertices[glyphVert + 0].texCoord = texCoord.minBound;
            outVertices[glyphVert + 3].atlasIdx = codeGlyph->texAtlasIdx;
            outVertices[glyphVert + 3].pos = { glyphLeft, glyphBottom };
            outVertices[glyphVert + 3].texCoord = { texCoord.minBound.x(), texCoord.maxBound.y() };
            // Add right edge 1 to 2
            outVertices[glyphVert + 1].atlasIdx = codeGlyph->texAtlasIdx;
            outVertices[glyphVert + 1].pos = { glyphRight, glyphTop };
            outVertices[glyphVert + 1].texCoord = { texCoord.maxBound.x(), texCoord.minBound.y() };
            outVertices[glyphVert + 2].atlasIdx = codeGlyph->texAtlasIdx;
            outVertices[glyphVert + 2].pos = { glyphRight, glyphBottom };
            outVertices[glyphVert + 2].texCoord = texCoord.maxBound;

            // Now advance to next word from horizontal start of this glyph
            lastWordWidth += int32(glyphToHeightScale * codeGlyph->advance);
            lastGlyph = codeGlyph;
        }
    }
    // If the last word needs to be auto wrapped? Wrapping it here
    if (cursorPos > 0 && lastWordVertex >= 0 && wrapWidth >= 0 && (cursorPos + lastWordWidth) > wrapWidth)
    {
        cursorPos = 0;
        wrapLastWord(outVertices, cursorPos);
    }
    // Below technique does not work as last word might be smaller than others
    // To grow BB we only have to add top left vertex of first letter and bottom right vertex of last
    // letter
    // outBB.grow(outVertices[0].pos);
    //// -2 since 3rd vertex is the bottom right of each letter
    // outBB.grow(outVertices[outVertices.size() - 2].pos);

    // Adding top left and bottom right of each glyph
    const uint32 endIdx = uint32(outVertices.size() / 4);
    for (uint32 idx = 0; idx < endIdx; ++idx)
    {
        const uint32 glyphVert = idx * 4;
        // Now add this quad to bounding box
        outBB.grow(outVertices[glyphVert + 0].pos);
        outBB.grow(outVertices[glyphVert + 2].pos);
    }
}
