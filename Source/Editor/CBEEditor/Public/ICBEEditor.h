/*!
 * \file ICBEEditor.h
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
#include "Modules/IModuleBase.h"
#include "EditorTypes.h"

class ICBEEditor : public IModuleBase
{
public:
    CBEEDITOR_EXPORT static ICBEEditor *get();

    virtual DelegateHandle addMenuDrawCallback(const TChar *menuName, ImGuiDrawInterfaceCallback::SingleCastDelegateType callback) const = 0;
    virtual void removeMenuDrawCallback(const TChar *menuName, DelegateHandle handle) const = 0;
};
