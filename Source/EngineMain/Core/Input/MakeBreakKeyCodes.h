#pragma once

namespace MakeBreakKeyCodes
{
    enum EKeyCode
    {
        KEY_ESC = 0x01,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_MINUS,//Add windows
        KEY_EQUAL,//Add windows
        KEY_BACKSPACE,
        KEY_TAB,
        KEY_Q,
        KEY_W,
        KEY_E,
        KEY_R,
        KEY_T,
        KEY_Y,
        KEY_U,
        KEY_I,
        KEY_O,
        KEY_P,
        KEY_OPEN_SQR,//Add windows
        KEY_CLOSE_SQR,// Add windows
        KEY_ENTER,
        KEY_LCTRL,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_SEMICOLON,// Add windows
        KEY_APOSTROPHE,// Add windows
        KEY_BACKTICK,// Add windows
        KEY_LSHIFT,
        KEY_BACKSLASH, // Add windows
        KEY_Z,
        KEY_X,
        KEY_C,
        KEY_V,
        KEY_B,
        KEY_N,
        KEY_M,
        KEY_COMMA,// Add windows
        KEY_FULLSTOP,// Add windows
        KEY_FWDSLASH,// Add windows
        KEY_RSHIFT,
        KEY_NUMASTERICK,// Add Windows
        KEY_LALT,
        KEY_SPACE,
        KEY_CAPS,
        KEY_F1,// Add Windows
        KEY_F2,// Add Windows
        KEY_F3,// Add Windows
        KEY_F4,// Add Windows
        KEY_F5,// Add Windows
        KEY_F6,// Add Windows
        KEY_F7,// Add Windows
        KEY_F8,// Add Windows
        KEY_F9,// Add Windows
        KEY_F10,// Add Windows
        KEY_NUMLOCK,// Add Windows
        KEY_SCRLLOCK,// Add Windows
        KEY_NUM7,// Add Windows
        KEY_NUM8,
        KEY_NUM9,// Add Windows 
        KEY_NUMMINUS,// Add Windows
        KEY_NUM4,// Add Windows
        KEY_NUM5,// Add Windows
        KEY_NUM6,// Add Windows
        KEY_NUMPLUS,// Add Windows
        KEY_NUM1,// Add Windows
        KEY_NUM2,// Add Windows
        KEY_NUM3,// Add Windows
        KEY_NUM0,// Add Windows
        KEY_NUMFULLSTOP,// Add Windows
        KEY_SYSREQ,// Add Windows
        KEY_F11,// Add Windows
        KEY_LEFTBCKSLASH,// Add Windows
        //KEY_F11,
        KEY_F12 = 0x58,// Add Windows
        //KEY_F15,
        KEY_PA1 = 0x5A,// Add Windows
        KEY_LWIN,// F13 // Add Windows
        KEY_RWIN,// F14 // Add Windows
        KEY_MENU,// F15 // Add Windows
        KEY_F16 = 0x63,// Add Windows
        KEY_F17,// Add Windows
        KEY_F18,// Add Windows
        KEY_F19,// Add Windows
        KEY_F20,// Add Windows
        KEY_F21,// Add Windows
        KEY_F22,// Add Windows
        KEY_F23,// Add Windows
        KEY_F24,// Add Windows
        KEY_FWDDEL = 0x70,// Add Windows
        KEY_NUMENTER = 0x74,// Add Windows
        KEY_CLR = 0x76,// Add Windows
        // Above 0xFF for application local
        MOUSE_LEFT = 0x100,
        MOUSE_RIGHT,
        MOUSE_MID,
        MOUSE_X1,
        MOUSE_X2,
        MOUSE_START = MOUSE_LEFT,// Add Windows
        MOUSE_END = MOUSE_X2,// Add Windows
        // E0 Keys
        E0_CODE = 0xE000,// Add windows
        KEY_RCTRL = E0_CODE | KEY_LCTRL,
        KEY_RALT = E0_CODE | KEY_LALT,
        KEY_NUMFWDSLASH = E0_CODE | KEY_FWDSLASH,// Add windows
        KEY_HOME = E0_CODE | KEY_NUM7,// Add windows
        KEY_UP = E0_CODE | KEY_NUM8,
        KEY_PAGEUP = E0_CODE | KEY_NUM9,// Add Windows 
        KEY_LEFT = E0_CODE | KEY_NUM4,
        KEY_RIGHT = E0_CODE | KEY_NUM6,
        KEY_END = E0_CODE | KEY_NUM1,// Add Windows
        KEY_DOWN = E0_CODE | KEY_NUM2,
        KEY_PAGEDOWN = E0_CODE | KEY_NUM3,// Add Windows
        KEY_INSERT = E0_CODE | KEY_NUM0,// Add Windows
        KEY_DELETE = E0_CODE | KEY_NUMFULLSTOP// Add Windows
    };
}
