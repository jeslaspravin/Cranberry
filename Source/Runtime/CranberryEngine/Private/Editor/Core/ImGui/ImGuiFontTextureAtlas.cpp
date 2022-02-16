/*!
 * \file ImGuiFontTextureAtlas.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ImGuiFontTextureAtlas.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "ImGuiLib/imgui.h"
#include "RenderInterface/GraphicsHelper.h"

ImGuiFontTextureAtlas* ImGuiFontTextureAtlas::createTexture(const ImGuiFontTextureParams& createParams)
{
    ImGuiFontTextureAtlas* texture = new ImGuiFontTextureAtlas();

    texture->owningContext = createParams.owningContext;
    texture->textureName = createParams.textureName;
    texture->dataFormat = EPixelDataFormat::R_U8_Norm;
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);
    texture->generateImGuiTexture();

    ImGuiFontTextureAtlas::init(texture);
    return texture;
}

void ImGuiFontTextureAtlas::destroyTexture(ImGuiFontTextureAtlas* textureAtlas)
{
    ImGuiFontTextureAtlas::destroy(textureAtlas);
    delete textureAtlas;
}

void ImGuiFontTextureAtlas::generateImGuiTexture()
{
    ImGui::SetCurrentContext(owningContext);
    ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;
    uint8* alphaVals;
    int32 textureSizeX, textureSizeY;
    fontAtlas->GetTexDataAsAlpha8(&alphaVals, &textureSizeX, &textureSizeY);

    textureSize = Size3D(textureSizeX, textureSizeY, 1);
    mipCount = (uint32)(1 + Math::floor(Math::log2((float)Math::max(textureSize.x, textureSize.y))));
    rawData.resize(textureSizeX * textureSizeY, ColorConst::BLACK);
    for (int32 i = 0; i < rawData.size(); ++i)
    {
        rawData[i].setR(alphaVals[i]);
    }
}

void ImGuiFontTextureAtlas::reinitResources()
{
    TextureBase::reinitResources();
    generateImGuiTexture();

    ENQUEUE_COMMAND(ReinitImGuiFontTextureAtlas)(
        [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                cmdList->copyToImage(textureResource, rawData);
            }
            else
            {
                ImGuiFontTextureAtlas::init(this);
            }
        });
}

void ImGuiFontTextureAtlas::init(ImGuiFontTextureAtlas* texture)
{
    ImageResourceCreateInfo imageCI
    {
        .imageFormat = texture->dataFormat,
        .dimensions = texture->textureSize,
        .numOfMips = texture->mipCount
    };

    ENQUEUE_COMMAND(InitImGuiFontTextureAtlas)(
        [texture, imageCI](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            texture->textureResource = graphicsHelper->createImage(graphicsInstance, imageCI);
            texture->textureResource->setResourceName(texture->textureName);
            texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
            texture->textureResource->setSampleCounts(texture->getSampleCount());

            texture->textureResource->init();
            cmdList->copyToImage(texture->textureResource, texture->rawData);
        });

}

void ImGuiFontTextureAtlas::destroy(ImGuiFontTextureAtlas* texture)
{
    ImageResourceRef textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyImGuiFontTextureAtlas)(
        [textureResource](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            textureResource->release();
        });

    texture->textureResource.reset();
}
