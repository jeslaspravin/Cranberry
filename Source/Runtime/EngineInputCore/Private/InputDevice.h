/*!
 * \file InputDevice.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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
    virtual bool sendInRaw(const void *rawInput) = 0;
    virtual void pullProcessedInputs(Keys *keyStates, AnalogStates *analogStates) = 0;
    virtual bool registerWindow(const GenericAppWindow *window) const = 0;
};
