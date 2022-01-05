/*!
 * \file EngineInputCoreModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Modules/IModuleBase.h"
#include "InputSystem.h"
#include "Types/Delegates/Delegate.h"

class ENGINEINPUTCORE_EXPORT EngineInputCoreModule : public IModuleBase
{
private:
    InputSystem inputSystem;
    DelegateHandle createdWindowHandle;
private:
    void createdNewWindow(GenericAppWindow* window) const;
public:
    InputSystem* getInputSystem() { return &inputSystem; }
    void updateInputs();

    /* IModuleBase overrides */
    void init() override;
    void release() override;
    /* Overrides end */
};