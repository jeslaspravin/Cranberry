#pragma once
#include "Keys.h"
#include "Memory/SmartPointers.h"
#include "EngineInputCoreExports.h"

class ENGINEINPUTCORE_EXPORT InputSystem
{
private:
    Keys keys;
    AnalogStates analogStates;

    class IRawInputBuffer* rawInputBuffer;
    SharedPtr<class IKeyToCharProcessor> keyToCharProcessor;
    std::vector<class IInputDevice*> inputDevices;
public:
    InputSystem();
    ~InputSystem();

    const KeyState* keyState(const Key& key) const;
    bool isKeyPressed(const Key& key) const;
    Utf32 keyChar(const Key& key) const;
    const InputAnalogState* analogState(AnalogStates::EStates stateKey) const;

    // When application going out of foreground
    void resetStates();
    void updateInputStates();
    void setKeyToCharProcessor(SharedPtr<class IKeyToCharProcessor> newKeyToCharProcessor);
    void registerWindow(const class GenericAppWindow* window) const;
};