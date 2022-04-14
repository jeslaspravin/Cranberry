/*!
 * \file IKeyToCharProcessor.h
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

class IKeyToCharProcessor
{
public:
    virtual ~IKeyToCharProcessor() = default;
    virtual void updateCharacters(class Keys *keyStates, class AnalogStates *analogStates) = 0;
    // Utf32 maybe while implementing localization I can change
    virtual Utf32 keyChar(Keys::StateKeyType key) const = 0;
};
