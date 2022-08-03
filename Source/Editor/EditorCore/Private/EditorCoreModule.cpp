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
#include "CBEObjectHelpers.h"
#include "AssetImporter.h"

IEditorCore *IEditorCore::get()
{
    static WeakModulePtr appModule = ModuleManager::get()->getOrLoadModule(TCHAR("EditorCore"));
    if (appModule.expired())
    {
        return nullptr;
    }
    return static_cast<IEditorCore *>(appModule.lock().get());
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
    for (AssetImporterBase *importer : importers)
    {
        if (importer->supportsImporting(inOutImport))
        {
            return importer;
        }
    }
    return nullptr;
}
