/*!
 * \file AssetLoaderLibrary.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/AssetLoaderLibrary.h"
#include "Types/Platform/LFS/PathFunctions.h"

EAssetType::Type AssetLoaderLibrary::typeFromAssetPath(const String &assetPath)
{
    String extension;
    PathFunctions::stripExtension(extension, assetPath);

    if (extension.startsWith(TCHAR("obj"), false))
    {
        return EAssetType::StaticMesh;
    }
    else if (extension.startsWith(
                 TCHAR("jpg"), false
                 )
                 || extension.startsWith(
                     TCHAR("jpeg"), false
                     )
                     || extension.startsWith(TCHAR("png"), false) || extension.startsWith(TCHAR("tga"), false))
    {
        return EAssetType::Texture2D;
    }
    else if (extension.startsWith(TCHAR("hdr"), false))
    {
        return EAssetType::CubeMap;
    }
    return EAssetType::InvalidType;
}
