/*!
 * \file CBEEditorModule.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Modules/ModuleManager.h"
#include "ICBEEditor.h"
#include "EditorEngine.h"
#include "Widgets/WgEditorImGuiLayer.h"
#include "StaticMeshImporter.h"
#include "IEditorCore.h"

ICBEEditor *ICBEEditor::get()
{
    static WeakModulePtr modulePtr = ModuleManager::get()->getOrLoadModule(TCHAR("CBEEditor"));
    if (modulePtr.expired())
    {
        return nullptr;
    }
    return static_cast<ICBEEditor *>(modulePtr.lock().get());
}

class CBEEditorModule : public ICBEEditor
{
public:
    /* IModuleBase overrides */
    void init() override;
    void release() override;

    DelegateHandle addMenuDrawCallback(const TChar *menuName, ImGuiDrawInterfaceCallback::SingleCastDelegateType &&callback) const override;
    void removeMenuDrawCallback(const TChar *menuName, DelegateHandle handle) const override;
    /* Override ends */
};

DECLARE_MODULE(CBEEditor, CBEEditorModule)

void CBEEditorModule::init()
{
    IEditorCore *editorCore = ModuleManager::get()->getOrLoadModulePtr<IEditorCore>(TCHAR("EditorCore"));
    debugAssert(editorCore);
    editorCore->registerAssetImporter(ObjStaticMeshImporter::staticType());
}

void CBEEditorModule::release()
{
    if (IEditorCore *editorCore = IEditorCore::get())
    {
        editorCore->unregisterAssetImporter(ObjStaticMeshImporter::staticType());
    }
}

DelegateHandle CBEEditorModule::addMenuDrawCallback(const TChar *menuName, ImGuiDrawInterfaceCallback::SingleCastDelegateType &&callback) const
{
    if (cbe::gCBEditorEngine && cbe::gCBEditorEngine->editorLayer)
    {
        return cbe::gCBEditorEngine->editorLayer->addMenuDrawExtender(menuName, std::forward<decltype(callback)>(callback));
    }
    return {};
}

void CBEEditorModule::removeMenuDrawCallback(const TChar *menuName, DelegateHandle handle) const
{
    if (cbe::gCBEditorEngine && cbe::gCBEditorEngine->editorLayer)
    {
        cbe::gCBEditorEngine->editorLayer->removeMenuExtender(menuName, handle);
    }
}
