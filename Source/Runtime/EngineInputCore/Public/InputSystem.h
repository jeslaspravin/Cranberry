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