/*!
 * \file RawInputBuffer.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "InputSystem/InputDevice.h"

struct ProcessInputsParam
{
    class Keys *keyStates;
    class AnalogStates *analogStates;
    IInputDeviceRef *inputDevices;
    int32 devicesNum;
};

class IRawInputBuffer
{
public:
    virtual ~IRawInputBuffer() = default;
    virtual void update() = 0;
    virtual void processInputs(const ProcessInputsParam &params) const = 0;
};