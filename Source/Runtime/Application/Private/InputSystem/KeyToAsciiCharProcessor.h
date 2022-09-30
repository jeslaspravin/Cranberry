/*!
 * \file KeyToAsciiCharProcessor.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "InputSystem/IKeyToCharProcessor.h"

class KeyToAsciiCharProcessor : public IKeyToCharProcessor
{
    struct KeyCharInfo
    {
        uint8 baseChar;
        uint8 shiftedChar = 0;
        AnalogStates::StateKeyType lockStateKey = AnalogStates::None;

        uint8 currentChar = 0;
    };

    std::map<Keys::StateKeyType, KeyCharInfo> keyToCharMap;

public:
    KeyToAsciiCharProcessor();

    /* IKeyToCharProcessor implementations */
    void updateCharacters(class Keys *keyStates, class AnalogStates *analogStates) override;
    Utf32 keyChar(Keys::StateKeyType key) const override;
    /* Overrides ends */
};