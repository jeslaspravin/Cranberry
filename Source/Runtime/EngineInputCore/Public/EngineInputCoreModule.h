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