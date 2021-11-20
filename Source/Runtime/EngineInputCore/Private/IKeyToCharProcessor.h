#pragma once
#include "Keys.h"

class IKeyToCharProcessor
{
public:
    virtual ~IKeyToCharProcessor() = default;
    virtual void updateCharacters(class Keys* keyStates, class AnalogStates* analogStates) = 0;
    // Utf32 maybe while implementing localization I can change
    virtual Utf32 keyChar(Keys::StateKeyType key) const = 0;
};
