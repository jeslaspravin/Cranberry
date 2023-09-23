/*!
 * \file AssetObject.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/Asset/AssetObject.h"

void AssetBase::setAssetName(const String &name) { assetHeader.assetName = name; }

const String &AssetBase::assetName() const { return assetHeader.assetName; }
