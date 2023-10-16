/*!
 * \file WindowsGamepadDevice.h
 *
 * \author Jeslas
 * \date July 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "InputSystem/InputDevice.h"

class WindowsGamepadDevice final : public IInputDevice
{
private:
public:
    WindowsGamepadDevice() = default;
    /* IInputDevice overrides */
    bool sendInRaw(const void *rawInput) final;
    bool registerWindow(const GenericAppWindow *window) const final;
    void pullProcessedInputs(Keys *keyStates, AnalogStates *analogStates) final;
    /* override ends */
};

namespace InputDevices
{
typedef WindowsGamepadDevice GamepadDevice;
}