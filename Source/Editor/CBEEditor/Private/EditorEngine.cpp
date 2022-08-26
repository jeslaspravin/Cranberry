/*!
 * \file EditorEngine.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EditorEngine.h"
#include "Widgets/WidgetWindow.h"
#include "Widgets/ImGui/WgImGui.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "IApplicationModule.h"
#include "ApplicationInstance.h"
#include "Widgets/WgEditorImGuiLayer.h"
#include "Widgets/WgViewportImGuiLayer.h"
#include "Widgets/WgWorldImGuiLayer.h"
#include "Widgets/WgDetailsImGuiLayer.h"
#include "Widgets/WgContentsImGuiLayer.h"
#include "Widgets/WgConsoleImGuiLayer.h"
#include "Classes/WorldsManager.h"
#include "WorldViewport.h"

namespace cbe
{

EditorEngine *gCBEditorEngine = nullptr;

EditorEngine::EditorEngine()
{
    if (BIT_NOT_SET(getFlags(), EObjectFlagBits::ObjFlag_Default))
    {
        debugAssert(gCBEditorEngine == nullptr);
        gCBEditorEngine = this;
    }
}

void EditorEngine::engineStart()
{
    WgImGui::WgArguments args;
    args.imguiManagerName = TCHAR("CBEd");
    args.bEnableDocking = true;
    wgImgui = std::make_shared<WgImGui>();
    wgImgui->construct(args);

    editorLayer = std::make_shared<WgEditorImGuiLayer>();
    viewportLayer = std::make_shared<WgViewportImGuiLayer>();
    worldLayer = std::make_shared<WgWorldImGuiLayer>();
    detailsLayer = std::make_shared<WgDetailsImGuiLayer>();
    consoleLayer = std::make_shared<WgConsoleImGuiLayer>();
    contentsLayer = std::make_shared<WgContentsImGuiLayer>();

    wgImgui->getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(editorLayer));
    wgImgui->getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(viewportLayer));
    wgImgui->getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(worldLayer));
    wgImgui->getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(detailsLayer));
    wgImgui->getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(consoleLayer));
    wgImgui->getImGuiManager().addLayer(std::static_pointer_cast<IImGuiLayer>(contentsLayer));
    IApplicationModule::get()->getApplication()->getMainWindow()->setContent(std::static_pointer_cast<WidgetBase>(wgImgui));

    worldInitHandle = gCBEEngine->worldManager()->onWorldInitEvent().bindLambda(
        [this](World *world, bool bIsMain)
        {
            if (bIsMain)
            {
                viewportLayer->setWorldViewport(std::make_shared<WorldViewport>(world));
            }
        }
    );
}

void EditorEngine::engineTick(float timeDelta) {}

void EditorEngine::engineExit()
{
    gCBEEngine->worldManager()->onWorldInitEvent().unbind(worldInitHandle);

    wgImgui.reset();
    editorLayer.reset();
    viewportLayer.reset();
    worldLayer.reset();
    detailsLayer.reset();
    contentsLayer.reset();
    consoleLayer.reset();
}

void EditorEngine::destroy() {}

} // namespace cbe