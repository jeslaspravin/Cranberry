#pragma once
#include "Keys.h"

class InputSystem
{
private:
    Keys keys;
    AnalogStates analogStates;

    class IRawInputBuffer* rawInputBuffer;
    std::vector<class IInputDevice*> inputDevices;
public:
    InputSystem();
    ~InputSystem();

    const KeyState* keyState(const Key& key) const;
    bool isKeyPressed(const Key& key) const;
    const InputAnalogState* analogState(AnalogStates::EStates stateKey) const;

    // When application going out of foreground
    void resetStates();
    void updateInputStates();
    void registerWindow(const class GenericAppWindow* window) const;
};