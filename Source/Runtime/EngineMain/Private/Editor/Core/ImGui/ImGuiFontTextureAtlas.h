/*!
 * \file ImGuiFontTextureAtlas.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Core/Types/Textures/TexturesBase.h"
#include "Types/Colors.h"

struct ImGuiContext;

struct ImGuiFontTextureParams : public TextureBaseCreateParams
{
    Color defaultColor = ColorConst::WHITE;
    ImGuiContext* owningContext;
};

// Texture 2Ds are texture that will be static and gets created from certain data
class ImGuiFontTextureAtlas : public TextureBase
{
private:
    Color defaultColor;
    ImGuiContext* owningContext;
    std::vector<Color> rawData;

public:
    static ImGuiFontTextureAtlas* createTexture(const ImGuiFontTextureParams& createParams);
    static void destroyTexture(ImGuiFontTextureAtlas* textureAtlas);
protected:
    ImGuiFontTextureAtlas() = default;
    ~ImGuiFontTextureAtlas() = default;

    void reinitResources() override;

private:
    static void init(ImGuiFontTextureAtlas* texture);
    static void destroy(ImGuiFontTextureAtlas* texture);

    void generateImGuiTexture();
};