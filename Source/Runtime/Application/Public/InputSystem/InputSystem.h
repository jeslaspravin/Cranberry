/*!
 * \file InputSystem.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "InputSystem/Keys.h"
#include "Memory/SmartPointers.h"
#include "InputSystem/InputDevice.h"

class InputSystem
{
private:
    Keys keys;
    AnalogStates analogStates;

    class IRawInputBuffer *rawInputBuffer;
    SharedPtr<class IKeyToCharProcessor> keyToCharProcessor;
    std::vector<IInputDeviceRef> inputDevices;

public:
    InputSystem();
    ~InputSystem();

    APPLICATION_EXPORT const KeyState *keyState(const Key &key) const;
    APPLICATION_EXPORT bool isKeyPressed(const Key &key) const;
    APPLICATION_EXPORT Utf32 keyChar(const Key &key) const;
    APPLICATION_EXPORT const InputAnalogState *analogState(AnalogStates::EStates stateKey) const;

    // When application going out of foreground
    void resetStates();
    void updateInputStates();

    APPLICATION_EXPORT void addInputDevice(IInputDeviceRef inputDevice);
    APPLICATION_EXPORT void setKeyToCharProcessor(SharedPtr<class IKeyToCharProcessor> newKeyToCharProcessor);
    APPLICATION_EXPORT void registerWindow(const class GenericAppWindow *window) const;
};