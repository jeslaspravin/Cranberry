/*!
 * \file EditorEngine.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEEditorExports.h"
#include "Classes/EngineBase.h"

#include "EditorEngine.gen.h"

class WgImGui;
class WgEditorImGuiLayer;
class WgViewportImGuiLayer;
class WgWorldImGuiLayer;
class WgDetailsImGuiLayer;
class WgContentsImGuiLayer;
class WgConsoleImGuiLayer;
class CBEEditorModule;

namespace cbe
{

class META_ANNOTATE_API(CBEEDITOR_EXPORT) EditorEngine : public EngineBase
{
    GENERATED_CODES()
private:
    friend CBEEditorModule;

    SharedPtr<WgImGui> wgImgui;
    SharedPtr<WgEditorImGuiLayer> editorLayer;
    SharedPtr<WgViewportImGuiLayer> viewportLayer;
    SharedPtr<WgWorldImGuiLayer> worldLayer;
    SharedPtr<WgDetailsImGuiLayer> detailsLayer;
    SharedPtr<WgContentsImGuiLayer> contentsLayer;
    SharedPtr<WgConsoleImGuiLayer> consoleLayer;

    DelegateHandle worldInitHandle;

public:
    EditorEngine();

    /* EngineBase overrides */
    void engineStart() override;
    void engineTick(float timeDelta) override;
    void engineExit() override;
    /* EngineBase overrides */
    void destroy() override;
    /* Override ends */
};

CBEEDITOR_EXPORT extern EditorEngine *gCBEditorEngine;

} // namespace cbe