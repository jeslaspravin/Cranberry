/*!
 * \file MakeBreakKeyCodes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
// based on https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
// Additional special scancodes https://www.scs.stanford.edu/10wi-cs140/pintos/specs/kbd/scancodes-5.html
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
    KEY_MINUS,
    KEY_EQUAL,
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
    KEY_OPEN_SQR,
    KEY_CLOSE_SQR,
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
    KEY_SEMICOLON,
    KEY_APOSTROPHE,
    KEY_BACKTICK,
    KEY_LSHIFT,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_FULLSTOP,
    KEY_FWDSLASH,
    KEY_RSHIFT,
    KEY_NUMASTERICK,
    KEY_LALT,
    KEY_SPACE,
    KEY_CAPS,
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
    KEY_NUMLOCK,
    KEY_SCRLLOCK,
    KEY_NUM7,
    KEY_NUM8,
    KEY_NUM9,
    KEY_NUMMINUS,
    KEY_NUM4,
    KEY_NUM5,
    KEY_NUM6,
    KEY_NUMPLUS,
    KEY_NUM1,
    KEY_NUM2,
    KEY_NUM3,
    KEY_NUM0,
    KEY_NUMFULLSTOP,
    KEY_SYSREQ,
    KEY_UNUSED0,
    KEY_LEFTBCKSLASH,
    KEY_F11,
    KEY_F12,
    KEY_PA1 = 0x5A,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16 = 0x63,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_FWDDEL = 0x70,
    // KEY_NUMENTER = 0x74, Don't know what this code, Maybe in old keyboards?
    KEY_CLR = 0x76,
    // Above 0xFF for application local as scan codes never goes above 128
    // E0 Keys, E0 is after 0xFF because These keys are denoted by additional flags and so it don't have
    // to be in [0x00, 0xFE]
    E0_CODE = 0x100,
    KEY_RCTRL = E0_CODE | KEY_LCTRL,
    KEY_RALT = E0_CODE | KEY_LALT,
    KEY_NUMFWDSLASH = E0_CODE | KEY_FWDSLASH,
    KEY_HOME = E0_CODE | KEY_NUM7,
    KEY_UP = E0_CODE | KEY_NUM8,
    KEY_PAGEUP = E0_CODE | KEY_NUM9,
    KEY_LEFT = E0_CODE | KEY_NUM4,
    KEY_RIGHT = E0_CODE | KEY_NUM6,
    KEY_END = E0_CODE | KEY_NUM1,
    KEY_DOWN = E0_CODE | KEY_NUM2,
    KEY_PAGEDOWN = E0_CODE | KEY_NUM3,
    KEY_INSERT = E0_CODE | KEY_NUM0,
    KEY_DELETE = E0_CODE | KEY_NUMFULLSTOP,
    KEY_NUMENTER = E0_CODE | KEY_ENTER,
    KEY_LWIN = E0_CODE | KEY_F13,
    KEY_RWIN = E0_CODE | KEY_F14,
    KEY_MENU = E0_CODE | KEY_F15,
    // E1 code Only pause/break so only one key after E0
    KEY_PAUSE,
    // Mouse key codes above 512 as they are not keys, we use them just to hold key state for mouse in
    // same data structure
    MOUSE_LEFT = 0x200,
    MOUSE_RIGHT,
    MOUSE_MID,
    MOUSE_X1,
    MOUSE_X2,
    MOUSE_START = MOUSE_LEFT,
    MOUSE_END = MOUSE_X2
};
} // namespace MakeBreakKeyCodes
