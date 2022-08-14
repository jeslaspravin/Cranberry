/*!
 * \file EditorCoreModule.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Modules/ModuleManager.h"
#include "IEditorCore.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "CBEObjectHelpers.h"
#include "AssetImporter.h"

IEditorCore *IEditorCore::get()
{
    static WeakModulePtr modulePtr = ModuleManager::get()->getOrLoadModule(TCHAR("EditorCore"));
    if (modulePtr.expired())
    {
        return nullptr;
    }
    return static_cast<IEditorCore *>(modulePtr.lock().get());
}

class EditorCoreModule final : public IEditorCore
{
private:
    std::vector<AssetImporterBase *> importers;

public:
    /* IModuleBase overrides */
    void init() final;
    void release() final;
    /* IEditorCore overrides */
    void registerAssetImporter(CBEClass importerClass) final;
    void unregisterAssetImporter(CBEClass importerClass) final;
    AssetImporterBase *findAssetImporter(ImportOption &inOutImport) final;

    /* Override ends */
};

DECLARE_MODULE(EditorCore, EditorCoreModule)

void EditorCoreModule::init() {}
void EditorCoreModule::release() {}
void EditorCoreModule::registerAssetImporter(CBEClass importerClass)
{
    if (importerClass == nullptr)
        return;
    AssetImporterBase *importer = cbe::cast<AssetImporterBase>(cbe::getDefaultObject(importerClass));
    if (importer && std::find(importers.cbegin(), importers.cend(), importer) == importers.cend())
    {
        importers.emplace_back(importer);
    }
}

void EditorCoreModule::unregisterAssetImporter(CBEClass importerClass) { std::erase(importers, cbe::getDefaultObject(importerClass)); }

AssetImporterBase *EditorCoreModule::findAssetImporter(ImportOption &inOutImport)
{
    if (inOutImport.fileExt.empty() || inOutImport.fileName.empty() || inOutImport.fileDirectory.empty())
    {
        inOutImport.fileDirectory = PathFunctions::splitFileAndDirectory(inOutImport.fileName, inOutImport.filePath);
        inOutImport.fileName = PathFunctions::stripExtension(inOutImport.fileExt, inOutImport.fileName);
    }
    for (AssetImporterBase *importer : importers)
    {
        if (importer->supportsImporting(inOutImport))
        {
            debugAssertf(inOutImport.optionsStruct, "Options must be filled even if it empty!");
            return importer;
        }
    }
    return nullptr;
}
