#include "InputSystem.h"

const KeyState* InputSystem::keyState(const Key& key) const
{
    return keys.queryKeyState(key);
}

bool InputSystem::isKeyPressed(const Key& key) const
{
    return keys.queryKeyState(key)->isPressed;
}

void InputSystem::resetToStart()
{
    keys.resetInit();
}

void InputSystem::clearInputs()
{
    keys.clear();
}

void InputSystem::updateInputStates()
{
    keys.pollInputs();
}

