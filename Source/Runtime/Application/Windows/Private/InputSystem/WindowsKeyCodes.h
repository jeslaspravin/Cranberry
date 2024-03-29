/*!
 * \file WindowsKeyCodes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

namespace EkeysKeyCode
{
enum EKeyCode
{
    MOUSE_LEFT = 0x01,
    MOUSE_RIGHT = 0x02,
    MOUSE_MID = 0x04,
    MOUSE_X1 = 0x05,
    MOUSE_X2 = 0x06,
    KEY_BACKSPACE = 0x08,
    KEY_TAB = 0x09,
    KEY_ENTER = 0x0D,
    KEY_CAPS = 0x14,
    KEY_ESC = 0x1B,
    KEY_SPACE = 0x20,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
    KEY_END,
    KEY_HOME,
    KEY_LEFT,
    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_INSERT = 0x2D,
    KEY_DELETE,
    KEY_0 = 0x30,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A = 0x41,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_NUM0 = 0x60,
    KEY_NUM1,
    KEY_NUM2,
    KEY_NUM3,
    KEY_NUM4,
    KEY_NUM5,
    KEY_NUM6,
    KEY_NUM7,
    KEY_NUM8,
    KEY_NUM9,
    KEY_NUMASTERICK,
    KEY_NUMPLUS,
    KEY_NUMMINUS = KEY_NUMPLUS + 2,
    KEY_NUMFULLSTOP,
    KEY_NUMFWDSLASH,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_LWIN, // F13
    KEY_RWIN, // F14
    KEY_MENU, // F15
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_NUMLOCK = 0x90,
    KEY_SCRLLOCK, // Add keys
    KEY_LSHIFT = 0xA0,
    KEY_RSHIFT,
    KEY_LCTRL,
    KEY_RCTRL,
    KEY_LALT,
    KEY_RALT,
    KEY_SEMICOLON = 0xBA,
    KEY_COMMA = 0xBC,
    KEY_FULLSTOP = 0xBE,
    KEY_FWDSLASH,
    KEY_MINUS,
    KEY_BACKTICK = 0xC0,
    KEY_OPEN_SQR = 0xDB, // Add keys
    KEY_CLOSE_SQR,
    KEY_BACKSLASH,
    KEY_APOSTROPHE,
    KEY_PA1 = 0xFD,
    KEY_CLR,
    // windows not specific key codes
    KEY_LEFTBCKSLASH = KEY_BACKSLASH,
    KEY_NUMENTER = KEY_ENTER,
    MOUSE_START = MOUSE_LEFT,
    MOUSE_END = MOUSE_X2,
    E0_CODE = 0,
    // Cannot find key codes anywhere
    KEY_EQUAL = 0x100,
    KEY_FWDDEL
};
} // namespace EkeysKeyCode

namespace KeyCode
{
typedef EkeysKeyCode::EKeyCode EKeyCode;
}