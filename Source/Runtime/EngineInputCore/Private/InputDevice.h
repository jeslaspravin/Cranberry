#pragma once
#include "Types/CoreTypes.h"

class Keys;
class AnalogStates;
class GenericAppWindow;

class IInputDevice
{
protected:
    constexpr static int8 UP_STATE = 1;
    constexpr static int8 DOWN_STATE = 0;
    constexpr static int8 INVALID_STATE = -1;
public:
    virtual bool sendInRaw(const void* rawInput) = 0;
    virtual void pullProcessedInputs(Keys* keyStates, AnalogStates* analogStates) = 0;
    virtual bool registerWindow(const GenericAppWindow* window) const = 0;
};
