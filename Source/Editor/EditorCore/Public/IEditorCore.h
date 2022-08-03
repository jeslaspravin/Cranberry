/*!
 * \file IEditorCore.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EditorCoreExports.h"
#include "Modules/IModuleBase.h"
#include "CBEObjectTypes.h"

class AssetImporterBase;
struct ImportOption;

class IEditorCore : public IModuleBase
{

public:
    EDITORCORE_EXPORT static IEditorCore *get();

    virtual void registerAssetImporter(CBEClass importerClass) = 0;
    virtual void unregisterAssetImporter(CBEClass importerClass) = 0;
    virtual AssetImporterBase *findAssetImporter(ImportOption &inOutImport) = 0;
};
