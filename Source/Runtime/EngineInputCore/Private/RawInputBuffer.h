#pragma once
#include "Types/CoreTypes.h"

struct ProcessInputsParam
{
    class Keys* keyStates;
    class AnalogStates* analogStates;
    class IInputDevice** inputDevices;
    int32 devicesNum;
};

class IRawInputBuffer
{
public:
    virtual ~IRawInputBuffer() = default;
    virtual void update() = 0;
    virtual void processInputs(const ProcessInputsParam& params) const = 0;
};