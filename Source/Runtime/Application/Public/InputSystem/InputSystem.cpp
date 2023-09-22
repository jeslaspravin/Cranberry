/*!
 * \file InputSystem.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "InputSystem/InputSystem.h"
#include "InputSystem/KeyToAsciiCharProcessor.h"
#include "InputSystem/PlatformInputTypes.h"

InputSystem::InputSystem()
{
    rawInputBuffer = new RawInputBuffer();
    inputDevices.emplace_back(new KeyboardDevice());
    inputDevices.emplace_back(new MouseDevice());
    inputDevices.emplace_back(new GamepadDevice());

    keyToCharProcessor.reset(new KeyToAsciiCharProcessor());
}

InputSystem::~InputSystem()
{
    delete rawInputBuffer;
    inputDevices.clear();
    keyToCharProcessor.reset();
}

const KeyState *InputSystem::keyState(const Key &key) const { return keys.queryState(key); }

bool InputSystem::isKeyPressed(const Key &key) const { return keys.queryState(key)->isPressed; }

Utf32 InputSystem::keyChar(const Key &key) const { return keyToCharProcessor->keyChar(&key); }

const InputAnalogState *InputSystem::analogState(AnalogStates::EStates stateKey) const { return analogStates.queryState(stateKey); }

void InputSystem::addInputDevice(IInputDeviceRef inputDevice) { inputDevices.emplace_back(inputDevice); }

void InputSystem::resetStates()
{
    keys.resetStates();
    analogStates.resetStates();
}

void InputSystem::updateInputStates()
{
    rawInputBuffer->update();

    ProcessInputsParam params;
    params.keyStates = &keys;
    params.analogStates = &analogStates;
    params.inputDevices = inputDevices.data();
    params.devicesNum = int32(inputDevices.size());
    rawInputBuffer->processInputs(params);

    keyToCharProcessor->updateCharacters(&keys, &analogStates);
}

void InputSystem::setKeyToCharProcessor(SharedPtr<class IKeyToCharProcessor> newKeyToCharProcessor)
{
    keyToCharProcessor = newKeyToCharProcessor;
}

void InputSystem::registerWindow(const class GenericAppWindow *window) const
{
    for (const IInputDeviceRef &device : inputDevices)
    {
        device->registerWindow(window);
    }
}
