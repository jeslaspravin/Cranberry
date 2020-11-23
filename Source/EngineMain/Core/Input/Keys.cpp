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
const Key Keys::PAGEUP{ EKeyCode::KEY_PAGEUP,"PageUp", ' ' };
const Key Keys::PAGEDOWN{ EKeyCode::KEY_PAGEDOWN,"PageDown", ' ' };
const Key Keys::END{ EKeyCode::KEY_END,"End", ' ' };
const Key Keys::HOME{ EKeyCode::KEY_HOME,"Home", ' ' };
const Key Keys::LEFT{ EKeyCode::KEY_LEFT,"Left Arrow", 0 };
const Key Keys::UP{ EKeyCode::KEY_UP,"Up Arrow", 0 };
const Key Keys::RIGHT{ EKeyCode::KEY_RIGHT,"Right Arrow", 0 };
const Key Keys::DOWN{ EKeyCode::KEY_DOWN,"Down Arrow", 0 };
const Key Keys::INS{ EKeyCode::KEY_INSERT,"Insert", 0 };
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
const Key Keys::G{ EKeyCode::KEY_G,"G", 'G' };
const Key Keys::H{ EKeyCode::KEY_H,"H", 'H' };
const Key Keys::I{ EKeyCode::KEY_I,"I", 'I' };
const Key Keys::J{ EKeyCode::KEY_J,"J", 'J' };
const Key Keys::K{ EKeyCode::KEY_K,"K", 'K' };
const Key Keys::L{ EKeyCode::KEY_L,"L", 'L' };
const Key Keys::M{ EKeyCode::KEY_M,"M", 'M' };
const Key Keys::N{ EKeyCode::KEY_N,"N", 'N' };
const Key Keys::O{ EKeyCode::KEY_O,"O", 'O' };
const Key Keys::P{ EKeyCode::KEY_P,"P", 'P' };
const Key Keys::Q{ EKeyCode::KEY_Q,"Q", 'Q' };
const Key Keys::R{ EKeyCode::KEY_R,"R", 'R' };
const Key Keys::S{ EKeyCode::KEY_S,"S", 'S' };
const Key Keys::T{ EKeyCode::KEY_T,"T", 'T' };
const Key Keys::U{ EKeyCode::KEY_U,"U", 'U' };
const Key Keys::V{ EKeyCode::KEY_V,"V", 'V' };
const Key Keys::W{ EKeyCode::KEY_W,"W", 'W' };
const Key Keys::X{ EKeyCode::KEY_X,"X", 'I' };
const Key Keys::Y{ EKeyCode::KEY_Y,"Y", 'Y' };
const Key Keys::Z{ EKeyCode::KEY_Z,"Z", 'Z' };
const Key Keys::NUM0{ EKeyCode::KEY_NUM0,"0", '0' };
const Key Keys::NUM1{ EKeyCode::KEY_NUM1,"1", '1' };
const Key Keys::NUM2{ EKeyCode::KEY_NUM2,"2", '2' };
const Key Keys::NUM3{ EKeyCode::KEY_NUM3,"3", '3' };
const Key Keys::NUM4{ EKeyCode::KEY_NUM4,"4", '4' };
const Key Keys::NUM5{ EKeyCode::KEY_NUM5,"5", '5' };
const Key Keys::NUM6{ EKeyCode::KEY_NUM6,"6", '6' };
const Key Keys::NUM7{ EKeyCode::KEY_NUM7,"7", '7' };
const Key Keys::NUM8{ EKeyCode::KEY_NUM8,"8", '8' };
const Key Keys::NUM9{ EKeyCode::KEY_NUM9,"9", '9' };
const Key Keys::ASTERICK{ EKeyCode::KEY_NUMASTERICK,"*", '*' };
const Key Keys::PLUS{ EKeyCode::KEY_NUMPLUS,"+", '+' };
const Key Keys::NUMMINUS{ EKeyCode::KEY_NUMMINUS,"-", '-' };
const Key Keys::NUMFULLSTOP{ EKeyCode::KEY_NUMFULLSTOP,".", '.' };
const Key Keys::NUMFWDSLASH{ EKeyCode::KEY_NUMFWDSLASH,"/", '/' };
const Key Keys::F1{ EKeyCode::KEY_F1,"F1", 0 };
const Key Keys::F2{ EKeyCode::KEY_F2,"F2", 0 };
const Key Keys::F3{ EKeyCode::KEY_F3,"F3", 0 };
const Key Keys::F4{ EKeyCode::KEY_F4,"F4", 0 };
const Key Keys::F5{ EKeyCode::KEY_F5,"F5", 0 };
const Key Keys::F6{ EKeyCode::KEY_F6,"F6", 0 };
const Key Keys::F7{ EKeyCode::KEY_F7,"F7", 0 };
const Key Keys::F8{ EKeyCode::KEY_F8,"F8", 0 };
const Key Keys::F9{ EKeyCode::KEY_F9,"F9", 0 };
const Key Keys::F10{ EKeyCode::KEY_F10,"F10", 0 };
const Key Keys::F11{ EKeyCode::KEY_F11,"F11", 0 };
const Key Keys::F12{ EKeyCode::KEY_F12,"F12", 0 };
const Key Keys::LWIN{ EKeyCode::KEY_LWIN,"LWIN", 0 };
const Key Keys::RWIN{ EKeyCode::KEY_RWIN,"RWIN", 0 };
const Key Keys::MENU{ EKeyCode::KEY_MENU,"MENU", 0 };
const Key Keys::F16{ EKeyCode::KEY_F16,"F16", 0 };
const Key Keys::F17{ EKeyCode::KEY_F17,"F17", 0 };
const Key Keys::F18{ EKeyCode::KEY_F18,"F18", 0 };
const Key Keys::F19{ EKeyCode::KEY_F19,"F19", 0 };
const Key Keys::F20{ EKeyCode::KEY_F20,"F20", 0 };
const Key Keys::F21{ EKeyCode::KEY_F21,"F21", 0 };
const Key Keys::F22{ EKeyCode::KEY_F22,"F22", 0 };
const Key Keys::F23{ EKeyCode::KEY_F23,"F23", 0 };
const Key Keys::F24{ EKeyCode::KEY_F24,"F24", 0 };
const Key Keys::NUMLOCK{ EKeyCode::KEY_NUMLOCK,"NUMLOCK", 0 };
const Key Keys::SCRLLOCK{ EKeyCode::KEY_SCRLLOCK,"SCROLLLOCK", 0 };
const Key Keys::LSHIFT{ EKeyCode::KEY_LSHIFT,"Left Shift", 0 };
const Key Keys::RSHIFT{ EKeyCode::KEY_RSHIFT,"Right Shift", 0 };
const Key Keys::LCTRL{ EKeyCode::KEY_LCTRL,"Left Control", 0 };
const Key Keys::RCTRL{ EKeyCode::KEY_RCTRL,"Right Control", 0 };
const Key Keys::LALT{ EKeyCode::KEY_LALT,"Left Alt", 0 };
const Key Keys::RALT{ EKeyCode::KEY_RALT,"Right Alt", 0 };
const Key Keys::SEMICOLON{ EKeyCode::KEY_SEMICOLON,";", ';' };
const Key Keys::COMMA{ EKeyCode::KEY_COMMA,",", ',' };
const Key Keys::FULLSTOP{ EKeyCode::KEY_FULLSTOP,".", '.' };
const Key Keys::FWDSLASH{ EKeyCode::KEY_FWDSLASH,"/", '/' };
const Key Keys::MINUS{ EKeyCode::KEY_MINUS,"-", '-' };
const Key Keys::BACKTICK{ EKeyCode::KEY_BACKTICK,"`", '`' };
const Key Keys::OPENSQR{ EKeyCode::KEY_OPEN_SQR,"[", '[' };
const Key Keys::CLOSESQR{ EKeyCode::KEY_CLOSE_SQR,"]", '[' };
const Key Keys::BACKSLASH{ EKeyCode::KEY_BACKSLASH,"\\", '\\' };
const Key Keys::APOSTROPHE{ EKeyCode::KEY_APOSTROPHE,"'", '\'' };
const Key Keys::PA1{ EKeyCode::KEY_PA1,"PA1", 0 };
const Key Keys::CLR{ EKeyCode::KEY_CLR,"CLR", 0 };
const Key Keys::LEFTBACKSLASH{ EKeyCode::KEY_LEFTBCKSLASH,"\\", '\\' };
const Key Keys::NUMENTER{ EKeyCode::KEY_NUMENTER,"Enter", '\r\n' };
const Key Keys::EQUAL{ EKeyCode::KEY_EQUAL,"=", '=' };
const Key Keys::FWDDEL{ EKeyCode::KEY_FWDDEL,"\b", '\b' };



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
    { ScrollWheelY, InputAnalogState() }
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
