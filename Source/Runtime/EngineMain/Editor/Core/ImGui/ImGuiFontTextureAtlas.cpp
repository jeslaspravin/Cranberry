#include "ImGuiFontTextureAtlas.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "ImGuiLib/imgui.h"

ImGuiFontTextureAtlas* ImGuiFontTextureAtlas::createTexture(const ImGuiFontTextureParams& createParams)
{
    ImGuiFontTextureAtlas* texture = new ImGuiFontTextureAtlas();

    texture->defaultColor = createParams.defaultColor;
    texture->owningContext = createParams.owningContext;
    texture->textureName = createParams.textureName;
    texture->dataFormat = EPixelDataFormat::BGRA_U8_Norm;
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
    rawData.resize(textureSizeX * textureSizeY);
    for (int32 i = 0; i < rawData.size(); ++i)
    {
        rawData[i] = defaultColor;
        rawData[i].setA(alphaVals[i]);
    }
}

void ImGuiFontTextureAtlas::reinitResources()
{
    TextureBase::reinitResources();
    generateImGuiTexture();

    ENQUEUE_COMMAND_NODEBUG(ReinitImGuiFontTextureAtlas,
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
        }, this);
}

void ImGuiFontTextureAtlas::init(ImGuiFontTextureAtlas* texture)
{
    texture->textureResource = new GraphicsImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setLayerCount(1);
    texture->textureResource->setNumOfMips(texture->mipCount);
    ENQUEUE_COMMAND_NODEBUG(InitImGuiFontTextureAtlas,
        {
            texture->textureResource->init();
            cmdList->copyToImage(texture->textureResource, texture->rawData);
        }
    , texture);

}

void ImGuiFontTextureAtlas::destroy(ImGuiFontTextureAtlas* texture)
{
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND_NODEBUG(DestroyImGuiFontTextureAtlas,
        {
            textureResource->release();
            delete textureResource;
        }, textureResource);

    texture->textureResource = nullptr;
}
