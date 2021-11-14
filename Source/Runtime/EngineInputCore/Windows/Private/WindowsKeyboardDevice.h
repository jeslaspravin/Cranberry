#pragma once
#include "InputDevice.h"

#include <map>


class WindowsKeyboardDevice final : public IInputDevice
{
private:
    std::map<uint32, int8> rawKeyStates;
    // Keys don't have analog states so no floats
    std::map<uint32, int8> analogRawStates;
public:

    /* IInputDevice overrides */
    bool sendInRaw(const void* rawInput) final;
    bool registerWindow(const GenericAppWindow* window) const final;
    void pullProcessedInputs(Keys* keyStates, AnalogStates* analogStates) final;

    /* override ends */
};

namespace InputDevices
{
    typedef WindowsKeyboardDevice KeyboardDevice;
}