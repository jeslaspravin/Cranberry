#include "AssetLoaderLibrary.h"
#include "../Core/Platform/LFS/PlatformLFS.h"


EAssetType::Type AssetLoaderLibrary::typeFromAssetPath(const String& assetPath)
{
    String extension;
    FileSystemFunctions::stripExtension(assetPath, extension);

    const AChar* extensionChar = extension.getChar();
    if (std::strcmp(extensionChar, "obj") == 0)
    {
        return EAssetType::StaticMesh;
    }
    else if (std::strcmp(extensionChar, "jpg") == 0 || std::strcmp(extensionChar, "jpeg") == 0 
        || std::strcmp(extensionChar, "png") == 0 || std::strcmp(extensionChar, "tga") == 0)
    {
        return EAssetType::Texture2D;
    }
    return EAssetType::InvalidType;
}
