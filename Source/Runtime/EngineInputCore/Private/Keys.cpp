/*!
 * \file Keys.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Keys.h"
#include "PlatformInputTypes.h"

const Key Keys::LMB{ EKeyCode::MOUSE_LEFT, TCHAR("Mouse Left") };
const Key Keys::RMB{ EKeyCode::MOUSE_RIGHT, TCHAR("Mouse Right") };
const Key Keys::MMB{ EKeyCode::MOUSE_MID, TCHAR("Mouse Middle") };
const Key Keys::X1MB{ EKeyCode::MOUSE_X1, TCHAR("Mouse X1") };
const Key Keys::X2MB{ EKeyCode::MOUSE_X2, TCHAR("Mouse X2") };
const Key Keys::BACKSPACE{ EKeyCode::KEY_BACKSPACE, TCHAR("Backspace") };
const Key Keys::TAB{ EKeyCode::KEY_TAB, TCHAR("Tab") };
const Key Keys::CAPS{ EKeyCode::KEY_CAPS, TCHAR("Caps lock") };
const Key Keys::ESC{ EKeyCode::KEY_ESC, TCHAR("Escape") };
const Key Keys::ENTER{ EKeyCode::KEY_ENTER, TCHAR("Enter") };
const Key Keys::SPACE{ EKeyCode::KEY_SPACE, TCHAR("Space") };
const Key Keys::PAGEUP{ EKeyCode::KEY_PAGEUP, TCHAR("PageUp") };
const Key Keys::PAGEDOWN{ EKeyCode::KEY_PAGEDOWN, TCHAR("PageDown") };
const Key Keys::END{ EKeyCode::KEY_END, TCHAR("End") };
const Key Keys::HOME{ EKeyCode::KEY_HOME, TCHAR("Home") };
const Key Keys::LEFT{ EKeyCode::KEY_LEFT, TCHAR("Left Arrow") };
const Key Keys::UP{ EKeyCode::KEY_UP, TCHAR("Up Arrow") };
const Key Keys::RIGHT{ EKeyCode::KEY_RIGHT, TCHAR("Right Arrow") };
const Key Keys::DOWN{ EKeyCode::KEY_DOWN, TCHAR("Down Arrow") };
const Key Keys::INS{ EKeyCode::KEY_INSERT, TCHAR("Insert") };
const Key Keys::DEL{ EKeyCode::KEY_DELETE, TCHAR("Delete") };
const Key Keys::ZERO{ EKeyCode::KEY_0, TCHAR("0") };
const Key Keys::ONE{ EKeyCode::KEY_1, TCHAR("1") };
const Key Keys::TWO{ EKeyCode::KEY_2, TCHAR("2") };
const Key Keys::THREE{ EKeyCode::KEY_3, TCHAR("3") };
const Key Keys::FOUR{ EKeyCode::KEY_4, TCHAR("4") };
const Key Keys::FIVE{ EKeyCode::KEY_5, TCHAR("5") };
const Key Keys::SIX{ EKeyCode::KEY_6, TCHAR("6") };
const Key Keys::SEVEN{ EKeyCode::KEY_7, TCHAR("7") };
const Key Keys::EIGHT{ EKeyCode::KEY_8, TCHAR("8") };
const Key Keys::NINE{ EKeyCode::KEY_9, TCHAR("9") };
const Key Keys::A{ EKeyCode::KEY_A, TCHAR("A") };
const Key Keys::B{ EKeyCode::KEY_B, TCHAR("B") };
const Key Keys::C{ EKeyCode::KEY_C, TCHAR("C") };
const Key Keys::D{ EKeyCode::KEY_D, TCHAR("D") };
const Key Keys::E{ EKeyCode::KEY_E, TCHAR("E") };
const Key Keys::F{ EKeyCode::KEY_F, TCHAR("F") };
const Key Keys::G{ EKeyCode::KEY_G, TCHAR("G") };
const Key Keys::H{ EKeyCode::KEY_H, TCHAR("H") };
const Key Keys::I{ EKeyCode::KEY_I, TCHAR("I") };
const Key Keys::J{ EKeyCode::KEY_J, TCHAR("J") };
const Key Keys::K{ EKeyCode::KEY_K, TCHAR("K") };
const Key Keys::L{ EKeyCode::KEY_L, TCHAR("L") };
const Key Keys::M{ EKeyCode::KEY_M, TCHAR("M") };
const Key Keys::N{ EKeyCode::KEY_N, TCHAR("N") };
const Key Keys::O{ EKeyCode::KEY_O, TCHAR("O") };
const Key Keys::P{ EKeyCode::KEY_P, TCHAR("P") };
const Key Keys::Q{ EKeyCode::KEY_Q, TCHAR("Q") };
const Key Keys::R{ EKeyCode::KEY_R, TCHAR("R") };
const Key Keys::S{ EKeyCode::KEY_S, TCHAR("S") };
const Key Keys::T{ EKeyCode::KEY_T, TCHAR("T") };
const Key Keys::U{ EKeyCode::KEY_U, TCHAR("U") };
const Key Keys::V{ EKeyCode::KEY_V, TCHAR("V") };
const Key Keys::W{ EKeyCode::KEY_W, TCHAR("W") };
const Key Keys::X{ EKeyCode::KEY_X, TCHAR("X") };
const Key Keys::Y{ EKeyCode::KEY_Y, TCHAR("Y") };
const Key Keys::Z{ EKeyCode::KEY_Z, TCHAR("Z") };
const Key Keys::NUM0{ EKeyCode::KEY_NUM0, TCHAR("0") };
const Key Keys::NUM1{ EKeyCode::KEY_NUM1, TCHAR("1") };
const Key Keys::NUM2{ EKeyCode::KEY_NUM2, TCHAR("2") };
const Key Keys::NUM3{ EKeyCode::KEY_NUM3, TCHAR("3") };
const Key Keys::NUM4{ EKeyCode::KEY_NUM4, TCHAR("4") };
const Key Keys::NUM5{ EKeyCode::KEY_NUM5, TCHAR("5") };
const Key Keys::NUM6{ EKeyCode::KEY_NUM6, TCHAR("6") };
const Key Keys::NUM7{ EKeyCode::KEY_NUM7, TCHAR("7") };
const Key Keys::NUM8{ EKeyCode::KEY_NUM8, TCHAR("8") };
const Key Keys::NUM9{ EKeyCode::KEY_NUM9, TCHAR("9") };
const Key Keys::ASTERICK{ EKeyCode::KEY_NUMASTERICK, TCHAR("*") };
const Key Keys::PLUS{ EKeyCode::KEY_NUMPLUS, TCHAR("+") };
const Key Keys::NUMMINUS{ EKeyCode::KEY_NUMMINUS, TCHAR("-") };
const Key Keys::NUMFULLSTOP{ EKeyCode::KEY_NUMFULLSTOP, TCHAR(".") };
const Key Keys::NUMFWDSLASH{ EKeyCode::KEY_NUMFWDSLASH, TCHAR("/") };
const Key Keys::F1{ EKeyCode::KEY_F1, TCHAR("F1") };
const Key Keys::F2{ EKeyCode::KEY_F2, TCHAR("F2") };
const Key Keys::F3{ EKeyCode::KEY_F3, TCHAR("F3") };
const Key Keys::F4{ EKeyCode::KEY_F4, TCHAR("F4") };
const Key Keys::F5{ EKeyCode::KEY_F5, TCHAR("F5") };
const Key Keys::F6{ EKeyCode::KEY_F6, TCHAR("F6") };
const Key Keys::F7{ EKeyCode::KEY_F7, TCHAR("F7") };
const Key Keys::F8{ EKeyCode::KEY_F8, TCHAR("F8") };
const Key Keys::F9{ EKeyCode::KEY_F9, TCHAR("F9") };
const Key Keys::F10{ EKeyCode::KEY_F10, TCHAR("F10") };
const Key Keys::F11{ EKeyCode::KEY_F11, TCHAR("F11") };
const Key Keys::F12{ EKeyCode::KEY_F12, TCHAR("F12") };
const Key Keys::LWIN{ EKeyCode::KEY_LWIN, TCHAR("LWIN") };
const Key Keys::RWIN{ EKeyCode::KEY_RWIN, TCHAR("RWIN") };
const Key Keys::MENU{ EKeyCode::KEY_MENU, TCHAR("MENU") };
const Key Keys::F16{ EKeyCode::KEY_F16, TCHAR("F16") };
const Key Keys::F17{ EKeyCode::KEY_F17, TCHAR("F17") };
const Key Keys::F18{ EKeyCode::KEY_F18, TCHAR("F18") };
const Key Keys::F19{ EKeyCode::KEY_F19, TCHAR("F19") };
const Key Keys::F20{ EKeyCode::KEY_F20, TCHAR("F20") };
const Key Keys::F21{ EKeyCode::KEY_F21, TCHAR("F21") };
const Key Keys::F22{ EKeyCode::KEY_F22, TCHAR("F22") };
const Key Keys::F23{ EKeyCode::KEY_F23, TCHAR("F23") };
const Key Keys::F24{ EKeyCode::KEY_F24, TCHAR("F24") };
const Key Keys::NUMLOCK{ EKeyCode::KEY_NUMLOCK, TCHAR("NUMLOCK") };
const Key Keys::SCRLLOCK{ EKeyCode::KEY_SCRLLOCK, TCHAR("SCROLLLOCK") };
const Key Keys::PAUSE{ EKeyCode::KEY_PAUSE, TCHAR("PAUSE/BREAK") };
const Key Keys::LSHIFT{ EKeyCode::KEY_LSHIFT, TCHAR("Left Shift") };
const Key Keys::RSHIFT{ EKeyCode::KEY_RSHIFT, TCHAR("Right Shift") };
const Key Keys::LCTRL{ EKeyCode::KEY_LCTRL, TCHAR("Left Control") };
const Key Keys::RCTRL{ EKeyCode::KEY_RCTRL, TCHAR("Right Control") };
const Key Keys::LALT{ EKeyCode::KEY_LALT, TCHAR("Left Alt") };
const Key Keys::RALT{ EKeyCode::KEY_RALT, TCHAR("Right Alt") };
const Key Keys::SEMICOLON{ EKeyCode::KEY_SEMICOLON, TCHAR(";") };
const Key Keys::COMMA{ EKeyCode::KEY_COMMA, TCHAR(", TCHAR(") };
const Key Keys::FULLSTOP{ EKeyCode::KEY_FULLSTOP, TCHAR(".") };
const Key Keys::FWDSLASH{ EKeyCode::KEY_FWDSLASH, TCHAR("/") };
const Key Keys::MINUS{ EKeyCode::KEY_MINUS, TCHAR("-") };
const Key Keys::BACKTICK{ EKeyCode::KEY_BACKTICK, TCHAR("`") };
const Key Keys::OPENSQR{ EKeyCode::KEY_OPEN_SQR, TCHAR("[") };
const Key Keys::CLOSESQR{ EKeyCode::KEY_CLOSE_SQR, TCHAR("]") };
const Key Keys::BACKSLASH{ EKeyCode::KEY_BACKSLASH, TCHAR("\\") };
const Key Keys::APOSTROPHE{ EKeyCode::KEY_APOSTROPHE, TCHAR("'") };
const Key Keys::PA1{ EKeyCode::KEY_PA1, TCHAR("PA1") };
const Key Keys::CLR{ EKeyCode::KEY_CLR, TCHAR("CLR") };
const Key Keys::LEFTBACKSLASH{ EKeyCode::KEY_LEFTBCKSLASH, TCHAR("\\") };
const Key Keys::NUMENTER{ EKeyCode::KEY_NUMENTER, TCHAR("Enter") };
const Key Keys::EQUAL{ EKeyCode::KEY_EQUAL, TCHAR("=") };
const Key Keys::FWDDEL{ EKeyCode::KEY_FWDDEL, TCHAR("\b") };



std::initializer_list<std::pair<const Key*, KeyState>> Keys::STATES_INITIALIZER
{
    { &LMB, KeyState()},
    { &RMB, KeyState()},
    { &MMB, KeyState()},
    { &X1MB, KeyState()},
    { &X2MB, KeyState()},
    { &BACKSPACE, KeyState()},
    { &TAB, KeyState()},
    { &CAPS, KeyState()},
    { &ESC, KeyState()},
    { &ENTER, KeyState()},
    { &SPACE, KeyState()},
    { &PAGEUP, KeyState()},
    { &PAGEDOWN, KeyState()},
    { &END, KeyState()},
    { &HOME, KeyState()},
    { &LEFT, KeyState()},
    { &UP, KeyState()},
    { &RIGHT, KeyState()},
    { &DOWN, KeyState()},
    { &INS, KeyState()},
    { &DEL, KeyState()},
    { &ZERO, KeyState()},
    { &ONE, KeyState()},
    { &TWO, KeyState()},
    { &THREE, KeyState()},
    { &FOUR, KeyState()},
    { &FIVE, KeyState()},
    { &SIX, KeyState()},
    { &SEVEN, KeyState()},
    { &EIGHT, KeyState()},
    { &NINE, KeyState()},
    { &A, KeyState()},
    { &B, KeyState()},
    { &C, KeyState()},
    { &D, KeyState()},
    { &E, KeyState()},
    { &F, KeyState()},
    { &G, KeyState()},
    { &H, KeyState()},
    { &I, KeyState()},
    { &J, KeyState()},
    { &K, KeyState()},
    { &L, KeyState()},
    { &M, KeyState()},
    { &N, KeyState()},
    { &O, KeyState()},
    { &P, KeyState()},
    { &Q, KeyState()},
    { &R, KeyState()},
    { &S, KeyState()},
    { &T, KeyState()},
    { &U, KeyState()},
    { &V, KeyState()},
    { &W, KeyState()},
    { &X, KeyState()},
    { &Y, KeyState()},
    { &Z, KeyState()},
    { &NUM0, KeyState()},
    { &NUM1, KeyState()},
    { &NUM2, KeyState()},
    { &NUM3, KeyState()},
    { &NUM4, KeyState()},
    { &NUM5, KeyState()},
    { &NUM6, KeyState()},
    { &NUM7, KeyState()},
    { &NUM8, KeyState()},
    { &NUM9, KeyState()},
    { &ASTERICK, KeyState()},
    { &PLUS, KeyState()},
    { &NUMMINUS, KeyState()},
    { &NUMFULLSTOP, KeyState()},
    { &NUMFWDSLASH, KeyState()},
    { &F1, KeyState()},
    { &F2, KeyState()},
    { &F3, KeyState()},
    { &F4, KeyState()},
    { &F5, KeyState()},
    { &F6, KeyState()},
    { &F7, KeyState()},
    { &F8, KeyState()},
    { &F9, KeyState()},
    { &F10, KeyState()},
    { &F11, KeyState()},
    { &F12, KeyState()},
    { &LWIN, KeyState()},
    { &RWIN, KeyState()},
    { &MENU, KeyState()},
    { &F16, KeyState()},
    { &F17, KeyState()},
    { &F18, KeyState()},
    { &F19, KeyState()},
    { &F20, KeyState()},
    { &F21, KeyState()},
    { &F22, KeyState()},
    { &F23, KeyState()},
    { &F24, KeyState()},
    { &NUMLOCK, KeyState()},
    { &SCRLLOCK, KeyState()},
    { &PAUSE, KeyState()},
    { &LSHIFT, KeyState()},
    { &RSHIFT, KeyState()},
    { &LCTRL, KeyState()},
    { &RCTRL, KeyState()},
    { &LALT, KeyState()},
    { &RALT, KeyState() },
    { &SEMICOLON, KeyState()},
    { &COMMA, KeyState()},
    { &FULLSTOP, KeyState()},
    { &FWDSLASH, KeyState()},
    { &MINUS, KeyState()},
    { &BACKTICK, KeyState()},
    { &OPENSQR, KeyState()},
    { &CLOSESQR, KeyState()},
    { &BACKSLASH, KeyState()},
    { &APOSTROPHE, KeyState()},
    { &PA1, KeyState()},
    { &CLR, KeyState()},
    { &LEFTBACKSLASH, KeyState()},
    { &NUMENTER, KeyState()},
    { &EQUAL, KeyState()},
    { &FWDDEL, KeyState()},
};


Keys::Keys()
    : keyStates(STATES_INITIALIZER.begin(), STATES_INITIALIZER.end())
{

}

const Keys::StateInfoType* Keys::queryState(const Key& key) const
{
    return &keyStates.at(&key);
}

std::map<Keys::StateKeyType, Keys::StateInfoType>& Keys::getKeyStates()
{
    return keyStates;
}

void Keys::resetStates()
{
    for (std::pair<const StateKeyType, StateInfoType>& keyStatePair : keyStates)
    {
        keyStatePair.second.isPressed = keyStatePair.second.keyWentUp = keyStatePair.second.keyWentDown = 0;
    }
}

bool Keys::isKeyboardKey(uint32 keyCode)
{
    return !isMouseKey(keyCode);
}

bool Keys::isMouseKey(uint32 keyCode)
{
    return EKeyCode::MOUSE_START <= keyCode && EKeyCode::MOUSE_END >= keyCode;
}

std::initializer_list<std::pair<AnalogStates::EStates, InputAnalogState>> AnalogStates::STATES_INITIALIZER
{
    { RelMouseX, InputAnalogState() },
    { RelMouseY, InputAnalogState() },
    { AbsMouseX, InputAnalogState() },
    { AbsMouseY, InputAnalogState() },
    { ScrollWheelX, InputAnalogState() },
    { ScrollWheelY, InputAnalogState() },
    { CapsLock, InputAnalogState() },
    { NumLock, InputAnalogState() },
    { ScrollLock, InputAnalogState() }
};

AnalogStates::AnalogStates()
    : analogStates(STATES_INITIALIZER.begin(), STATES_INITIALIZER.end())
{}

const AnalogStates::StateInfoType* AnalogStates::queryState(EStates analogState) const
{
    auto stateItr = analogStates.find(analogState);
    if (stateItr != analogStates.cend())
    {
        return &stateItr->second;
    }
    return nullptr;
}

std::map<AnalogStates::StateKeyType, AnalogStates::StateInfoType>& AnalogStates::getAnalogStates()
{
    return analogStates;
}

void AnalogStates::resetStates()
{
    for (std::pair<const EStates, InputAnalogState>& analogStatePair : analogStates)
    {
        analogStatePair.second.acceleration = analogStatePair.second.currentValue = analogStatePair.second.startedThisFrame =
            analogStatePair.second.startedThisFrame = 0;
    }
}
