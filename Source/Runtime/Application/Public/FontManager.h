/*!
 * \file FontManager.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "Math/Box.h"
#include "Types/Delegates/Delegate.h"
#include "ApplicationExports.h"

class ShaderParameters;

class FontManager
{
private:
    class FontManagerContext* context = nullptr;
public:
    using FontIndex = uint8;

    using FontEvent = Event<FontManager, FontIndex>;
    using FontManagerEvent = SimpleEvent<FontManager>;

    FontEvent onFontAdded;
    FontManagerEvent preTextureAtlasUpdate;
    FontManagerEvent textureAltasUpdated;
private:
    void addGlyphsFromStr(const String& str, FontIndex font, uint32 height) const;
public:
    FontManager() = default;
    // Actual initialization
    FontManager(EInitType);
    FontManager(const FontManager& otherManager) = default;
    FontManager(FontManager&& otherManager);
    FontManager& operator=(const FontManager& otherManager) = default;
    FontManager& operator=(FontManager && otherManager);
    ~FontManager();

    void broadcastPreTextureAtlasUpdate() const { preTextureAtlasUpdate.invoke(); }
    void broadcastTextureAtlasUpdated() const { textureAltasUpdated.invoke(); }

    APPLICATION_EXPORT FontIndex addFont(const String& fontPath) const;
    APPLICATION_EXPORT FontIndex addFont(const std::vector<uint8>& fontData, const String& fontName) const;
    /**
    * FontManager::addGlpyhs - Adds glyphs of a font to the glyph build list, This needs to be called and glyphs must be added 
    * before querying or drawing any texts/glyphs
    *
    * Access: public  
    *
    * @param FontIndex font - Font to generate glyph from
    * @param const std::vector<ValueRange<uint32>> & glyphCodeRanges - Glyph's codepoint ranges, start of range is inclusive and end is exclusive
    * @param const std::vector<uint32> & heights - All height variations of given glyphs to generate
    *
    * @return void
    */
    APPLICATION_EXPORT void addGlyphs(FontIndex font, const std::vector<ValueRange<uint32>>& glyphCodeRanges, const std::vector<uint32>& heights) const;
    APPLICATION_EXPORT void addGlyphs(FontIndex font, const ValueRange<uint32>& glyphCodeRange, uint32 height) const;

    APPLICATION_EXPORT void setupTextureAtlas(ShaderParameters* shaderParams, const String& paramName);

    APPLICATION_EXPORT uint32 calculateRenderWidth(const String& text, FontIndex font, uint32 height) const;
    // If wrap width is -1 no wrapping will be calculated
    APPLICATION_EXPORT uint32 calculateRenderHeight(const String& text, FontIndex font, uint32 height, int32 wrapWidth = -1) const;
};