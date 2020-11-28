#include "InputSystem.h"
#include "PlatformInputTypes.h"
#include "KeyToAsciiCharProcessor.h"

InputSystem::InputSystem()
{
    rawInputBuffer = new RawInputBuffer();
    inputDevices.emplace_back(new KeyboardDevice());
    inputDevices.emplace_back(new MouseDevice());

    keyToCharProcessor.reset(new KeyToAsciiCharProcessor());
}

InputSystem::~InputSystem()
{
    delete rawInputBuffer;
    for (IInputDevice* inputDevice : inputDevices)
    {
        delete inputDevice;
    }
    inputDevices.clear();
    keyToCharProcessor.reset();
}

const KeyState* InputSystem::keyState(const Key& key) const
{
    return keys.queryState(key);
}

bool InputSystem::isKeyPressed(const Key& key) const
{
    return keys.queryState(key)->isPressed;
}

Utf32 InputSystem::keyChar(const Key& key) const
{
    return keyToCharProcessor->keyChar(&key);
}

const InputAnalogState* InputSystem::analogState(AnalogStates::EStates stateKey) const
{
    return analogStates.queryState(stateKey);
}

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

void InputSystem::registerWindow(const class GenericAppWindow* window) const
{
    for (const IInputDevice* device : inputDevices)
    {
        device->registerWindow(window);
    }
}

