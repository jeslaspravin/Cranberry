#pragma once
#include "InputDevice.h"

class WindowsMouseDevice final : public IInputDevice
{
private:
    std::map<uint32, int8> buttonRawStates;
    std::map<uint32, float> analogRawStates;
public:
    WindowsMouseDevice();
    /* IInputDevice overrides */
    bool sendInRaw(const void* rawInput) final;
    bool registerWindow(const GenericAppWindow* window) const final;
    void pullProcessedInputs(Keys* keyStates, AnalogStates* analogStates) final;
    /* override ends */
};

namespace InputDevices
{
    typedef WindowsMouseDevice MouseDevice;
}