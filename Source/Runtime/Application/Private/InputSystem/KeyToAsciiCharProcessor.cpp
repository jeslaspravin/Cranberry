/*!
 * \file KeyToAsciiCharProcessor.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "InputSystem/KeyToAsciiCharProcessor.h"

KeyToAsciiCharProcessor::KeyToAsciiCharProcessor()
{
    keyToCharMap[&Keys::BACKSPACE] = { '\b', '\b' };
    keyToCharMap[&Keys::TAB] = { '\t', '\t' };
    keyToCharMap[&Keys::ENTER] = { '\n', '\n' };
    keyToCharMap[&Keys::SPACE] = { ' ', ' ' };
    keyToCharMap[&Keys::DEL] = { 127, 127 };
    keyToCharMap[&Keys::ZERO] = { '0', ')' };
    keyToCharMap[&Keys::ONE] = { '1', '!' };
    keyToCharMap[&Keys::TWO] = { '2', '@' };
    keyToCharMap[&Keys::THREE] = { '3', '#' };
    keyToCharMap[&Keys::FOUR] = { '4', '$' };
    keyToCharMap[&Keys::FIVE] = { '5', '%' };
    keyToCharMap[&Keys::SIX] = { '6', '^' };
    keyToCharMap[&Keys::SEVEN] = { '7', '&' };
    keyToCharMap[&Keys::EIGHT] = { '8', '*' };
    keyToCharMap[&Keys::NINE] = { '9', '(' };
    keyToCharMap[&Keys::A] = { 'a', 'A', AnalogStates::CapsLock };
    keyToCharMap[&Keys::B] = { 'b', 'B', AnalogStates::CapsLock };
    keyToCharMap[&Keys::C] = { 'c', 'C', AnalogStates::CapsLock };
    keyToCharMap[&Keys::D] = { 'd', 'D', AnalogStates::CapsLock };
    keyToCharMap[&Keys::E] = { 'e', 'E', AnalogStates::CapsLock };
    keyToCharMap[&Keys::F] = { 'f', 'F', AnalogStates::CapsLock };
    keyToCharMap[&Keys::G] = { 'g', 'G', AnalogStates::CapsLock };
    keyToCharMap[&Keys::H] = { 'h', 'H', AnalogStates::CapsLock };
    keyToCharMap[&Keys::I] = { 'i', 'I', AnalogStates::CapsLock };
    keyToCharMap[&Keys::J] = { 'j', 'J', AnalogStates::CapsLock };
    keyToCharMap[&Keys::K] = { 'k', 'K', AnalogStates::CapsLock };
    keyToCharMap[&Keys::L] = { 'l', 'L', AnalogStates::CapsLock };
    keyToCharMap[&Keys::M] = { 'm', 'M', AnalogStates::CapsLock };
    keyToCharMap[&Keys::N] = { 'n', 'N', AnalogStates::CapsLock };
    keyToCharMap[&Keys::O] = { 'o', 'O', AnalogStates::CapsLock };
    keyToCharMap[&Keys::P] = { 'p', 'P', AnalogStates::CapsLock };
    keyToCharMap[&Keys::Q] = { 'q', 'Q', AnalogStates::CapsLock };
    keyToCharMap[&Keys::R] = { 'r', 'R', AnalogStates::CapsLock };
    keyToCharMap[&Keys::S] = { 's', 'S', AnalogStates::CapsLock };
    keyToCharMap[&Keys::T] = { 't', 'T', AnalogStates::CapsLock };
    keyToCharMap[&Keys::U] = { 'u', 'U', AnalogStates::CapsLock };
    keyToCharMap[&Keys::V] = { 'v', 'V', AnalogStates::CapsLock };
    keyToCharMap[&Keys::W] = { 'w', 'W', AnalogStates::CapsLock };
    keyToCharMap[&Keys::X] = { 'x', 'X', AnalogStates::CapsLock };
    keyToCharMap[&Keys::Y] = { 'y', 'Y', AnalogStates::CapsLock };
    keyToCharMap[&Keys::Z] = { 'z', 'Z', AnalogStates::CapsLock };
    keyToCharMap[&Keys::NUM0] = { 0, '0', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM1] = { 0, '1', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM2] = { 0, '2', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM3] = { 0, '3', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM4] = { 0, '4', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM5] = { 0, '5', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM6] = { 0, '6', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM7] = { 0, '7', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM8] = { 0, '8', AnalogStates::NumLock };
    keyToCharMap[&Keys::NUM9] = { 0, '9', AnalogStates::NumLock };
    keyToCharMap[&Keys::ASTERICK] = { '*', '*' };
    keyToCharMap[&Keys::PLUS] = { '+', '+' };
    keyToCharMap[&Keys::NUMMINUS] = { '-', '-' };
    keyToCharMap[&Keys::NUMFULLSTOP] = { '.', '.' };
    keyToCharMap[&Keys::NUMFWDSLASH] = { '/', '/' };
    keyToCharMap[&Keys::SEMICOLON] = { ';', ':' };
    keyToCharMap[&Keys::COMMA] = { ',', '<' };
    keyToCharMap[&Keys::FULLSTOP] = { '.', '>' };
    keyToCharMap[&Keys::FWDSLASH] = { '/', '?' };
    keyToCharMap[&Keys::MINUS] = { '-', '_' };
    keyToCharMap[&Keys::BACKTICK] = { '`', '~' };
    keyToCharMap[&Keys::OPENSQR] = { '[', '{' };
    keyToCharMap[&Keys::CLOSESQR] = { ']', '}' };
    keyToCharMap[&Keys::BACKSLASH] = { '\\', '|' };
    keyToCharMap[&Keys::APOSTROPHE] = { '\'', '"' };
    keyToCharMap[&Keys::LEFTBACKSLASH] = { '\\', '|' };
    keyToCharMap[&Keys::NUMENTER] = { '\n', '\n' };
    keyToCharMap[&Keys::EQUAL] = { '=', '+' };
    keyToCharMap[&Keys::FWDDEL] = { '\b', '\b' };
}

void KeyToAsciiCharProcessor::updateCharacters(class Keys *keyStates, class AnalogStates *analogStates)
{
    bool bShiftDown = keyStates->queryState(Keys::LSHIFT)->isPressed || keyStates->queryState(Keys::RSHIFT)->isPressed;
    for (std::pair<const Keys::StateKeyType, KeyCharInfo> &keyCharInfo : keyToCharMap)
    {
        if (keyStates->queryState(*keyCharInfo.first)->isPressed)
        {
            bool bKeyShifted = bShiftDown;
            if (keyCharInfo.second.lockStateKey != AnalogStates::None)
            {
                bKeyShifted = analogStates->queryState(keyCharInfo.second.lockStateKey)->currentValue > 0.0f ? !bKeyShifted : bKeyShifted;
            }

            keyCharInfo.second.currentChar = bKeyShifted ? keyCharInfo.second.shiftedChar : keyCharInfo.second.baseChar;
        }
        else
        {
            keyCharInfo.second.currentChar = 0;
        }
    }
}

Utf32 KeyToAsciiCharProcessor::keyChar(Keys::StateKeyType key) const
{
    auto keyToCharItr = keyToCharMap.find(key);
    if (keyToCharItr != keyToCharMap.cend())
    {
        return keyToCharItr->second.currentChar;
    }
    return 0;
}
