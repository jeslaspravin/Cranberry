/*!
 * \file WindowsMouseDevice.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "InputSystem/InputDevice.h"

class WindowsMouseDevice final : public IInputDevice
{
private:
    std::map<uint32, int8> buttonRawStates;
    std::map<uint32, float> analogRawStates;
    /* In virtual desktop relative move is not getting published.
     * In normal desktop absolute position is not getting published.
     * Below flags will be used to update the respective other, If one is updated and other is not */
    bool bRelMoveUpdated = false;
    bool bAbsPosUpdated = false;

public:
    WindowsMouseDevice();
    /* IInputDevice overrides */
    bool sendInRaw(const void *rawInput) final;
    bool registerWindow(const GenericAppWindow *window) const final;
    void pullProcessedInputs(Keys *keyStates, AnalogStates *analogStates) final;
    /* override ends */
};

namespace InputDevices
{
typedef WindowsMouseDevice MouseDevice;
}