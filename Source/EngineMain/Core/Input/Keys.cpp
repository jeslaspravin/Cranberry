#include "Keys.h"
#include "PlatformInputTypes.h"

const Key Keys::LMB{ EKeyCode::MOUSE_LEFT,"Mouse Left", 0 };
const Key Keys::RMB{ EKeyCode::MOUSE_RIGHT,"Mouse Right", 0 };
const Key Keys::MMB{ EKeyCode::MOUSE_MID,"Mouse Middle", 0 };
const Key Keys::X1MB{ EKeyCode::MOUSE_X1,"Mouse X1", 0 };
const Key Keys::X2MB{ EKeyCode::MOUSE_X2,"Mouse X2", 0 };
const Key Keys::BACKSPACE{ EKeyCode::KEY_BACKSPACE,"Backspace", '\b' };
const Key Keys::TAB{ EKeyCode::KEY_TAB,"Tab", '\t' };
const Key Keys::CAPS{ EKeyCode::KEY_CAPS,"Caps lock", 0 };
const Key Keys::ESC{ EKeyCode::KEY_ESC,"Escape", 0 };
const Key Keys::ENTER{ EKeyCode::KEY_ENTER,"Enter", '\r\n' };
const Key Keys::SPACE{ EKeyCode::KEY_SPACE,"Space", ' ' };
const Key Keys::LEFT{ EKeyCode::KEY_LEFT,"Left Arrow", 0 };
const Key Keys::UP{ EKeyCode::KEY_UP,"Up Arrow", 0 };
const Key Keys::RIGHT{ EKeyCode::KEY_RIGHT,"Right Arrow", 0 };
const Key Keys::DOWN{ EKeyCode::KEY_DOWN,"Down Arrow", 0 };
const Key Keys::DEL{ EKeyCode::KEY_DELETE,"Delete", 127 };
const Key Keys::ZERO{ EKeyCode::KEY_0,"0", '0' };
const Key Keys::ONE{ EKeyCode::KEY_1,"1", '1' };
const Key Keys::TWO{ EKeyCode::KEY_2,"2", '2' };
const Key Keys::THREE{ EKeyCode::KEY_3,"3", '3' };
const Key Keys::FOUR{ EKeyCode::KEY_4,"4", '4' };
const Key Keys::FIVE{ EKeyCode::KEY_5,"5", '5' };
const Key Keys::SIX{ EKeyCode::KEY_6,"6", '6' };
const Key Keys::SEVEN{ EKeyCode::KEY_7,"7", '7' };
const Key Keys::EIGHT{ EKeyCode::KEY_8,"8", '8' };
const Key Keys::NINE{ EKeyCode::KEY_9,"9", '9' };
const Key Keys::A{ EKeyCode::KEY_A,"A", 'A' };
const Key Keys::B{ EKeyCode::KEY_B,"B", 'B' };
const Key Keys::C{ EKeyCode::KEY_C,"C", 'C' };
const Key Keys::D{ EKeyCode::KEY_D,"D", 'D' };
const Key Keys::E{ EKeyCode::KEY_E,"E", 'E' };
const Key Keys::F{ EKeyCode::KEY_F,"F", 'F' };
const Key Keys::P{ EKeyCode::KEY_P,"P", 'P' };
const Key Keys::Q{ EKeyCode::KEY_Q,"Q", 'Q' };
const Key Keys::R{ EKeyCode::KEY_R,"R", 'R' };
const Key Keys::S{ EKeyCode::KEY_S,"S", 'S' };
const Key Keys::W{ EKeyCode::KEY_W,"W", 'W' };
const Key Keys::X{ EKeyCode::KEY_X,"X", 'I' };
const Key Keys::Y{ EKeyCode::KEY_Y,"Y", 'Y' };
const Key Keys::Z{ EKeyCode::KEY_Z,"Z", 'Z' };
const Key Keys::LSHIFT{ EKeyCode::KEY_LSHIFT,"Left Shift", 0 };
const Key Keys::RSHIFT{ EKeyCode::KEY_RSHIFT,"Right Shift", 0 };
const Key Keys::LCTRL{ EKeyCode::KEY_LCTRL,"Left Control", 0 };
const Key Keys::RCTRL{ EKeyCode::KEY_RCTRL,"Right Control", 0 };
const Key Keys::LALT{ EKeyCode::KEY_LALT,"Left Alt", 0 };
const Key Keys::RALT{ EKeyCode::KEY_RALT,"Right Alt", 0 };

std::initializer_list<std::pair<const Key*, KeyState>> Keys::KEYSTATES_INITIALIZER
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
    { &LEFT, KeyState()},
    { &UP, KeyState()},
    { &RIGHT, KeyState()},
    { &DOWN, KeyState()},
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
    { &P, KeyState()},
    { &Q, KeyState()},
    { &R, KeyState()},
    { &S, KeyState()},
    { &W, KeyState()},
    { &X, KeyState()},
    { &Y, KeyState()},
    { &Z, KeyState()},
    { &LSHIFT, KeyState()},
    { &RSHIFT, KeyState()},
    { &LCTRL, KeyState()},
    { &RCTRL, KeyState()},
    { &LALT, KeyState()},
    { &RALT, KeyState()},
};


Keys::Keys()
    : keyStates(KEYSTATES_INITIALIZER.begin(), KEYSTATES_INITIALIZER.end())
{

}

const KeyState* Keys::queryState(const Key& key) const
{
    return &keyStates.at(&key);
}

std::map<const Key*, KeyState>& Keys::getKeyStates()
{
    return keyStates;
}

void Keys::resetStates()
{
    for (std::pair<Key const* const, KeyState>& keyStatePair : keyStates)
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

std::initializer_list<std::pair<AnalogStates::EStates, InputAnalogState>> AnalogStates::ANALOGSTATES_INITIALIZER
{
    { RelMouseX, InputAnalogState() },
    { RelMouseX, InputAnalogState() },
    { AbsMouseX, InputAnalogState() },
    { AbsMouseX, InputAnalogState() },
    { ScrollWheelX, InputAnalogState() },
    { ScrollWheelY, InputAnalogState() }
};

AnalogStates::AnalogStates()
    : analogStates(ANALOGSTATES_INITIALIZER.begin(), ANALOGSTATES_INITIALIZER.end())
{}

const InputAnalogState* AnalogStates::queryState(EStates analogState) const
{
    auto stateItr = analogStates.find(analogState);
    if (stateItr != analogStates.cend())
    {
        return &stateItr->second;
    }
    return nullptr;
}

std::map<AnalogStates::EStates, InputAnalogState>& AnalogStates::getAnalogStates()
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
