/*!
 * \file WindowsDevices.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "GenericAppWindow.h"
#include "InputSystem/Keys.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "InputSystem/PlatformInputTypes.h"
#include "WindowsCommonHeaders.h"

#include <hidusage.h>

//////////////////////////////////////////////////////////////////////////
// Mouse device
//////////////////////////////////////////////////////////////////////////

WindowsMouseDevice::WindowsMouseDevice()
{
    analogRawStates[AnalogStates::RelMouseX] = analogRawStates[AnalogStates::RelMouseY] = 0;
    analogRawStates[AnalogStates::ScrollWheelX] = analogRawStates[AnalogStates::ScrollWheelY] = 0;
}

bool WindowsMouseDevice::sendInRaw(const void *rawInput)
{
    const RAWINPUT *winRawInput = reinterpret_cast<const RAWINPUT *>(rawInput);
    if (winRawInput->header.dwType != RIM_TYPEMOUSE)
    {
        return false;
    }

    const RAWMOUSE &mouseData = winRawInput->data.mouse;
    if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_1_UP)) != 0) // LMB
    {
        buttonRawStates[EKeyCode::MOUSE_LEFT]
            = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) == RI_MOUSE_BUTTON_1_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_2_DOWN | RI_MOUSE_BUTTON_2_UP)) != 0) // RMB
    {
        buttonRawStates[EKeyCode::MOUSE_RIGHT]
            = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) == RI_MOUSE_BUTTON_2_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_3_DOWN | RI_MOUSE_BUTTON_3_UP)) != 0) // MMB
    {
        buttonRawStates[EKeyCode::MOUSE_MID]
            = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) == RI_MOUSE_BUTTON_3_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_4_UP)) != 0) // X1MB
    {
        buttonRawStates[EKeyCode::MOUSE_X1]
            = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) == RI_MOUSE_BUTTON_5_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_5_UP)) != 0) // X2MB
    {
        buttonRawStates[EKeyCode::MOUSE_X2]
            = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) == RI_MOUSE_BUTTON_5_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL)
    {
        analogRawStates[AnalogStates::ScrollWheelY] = float(int16(mouseData.usButtonData)) / WHEEL_DELTA;
    }
    else if ((mouseData.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL)
    {
        analogRawStates[AnalogStates::ScrollWheelX] = float(int16(mouseData.usButtonData)) / WHEEL_DELTA;
    }

    if (BIT_SET(mouseData.usFlags, MOUSE_MOVE_ABSOLUTE))
    {
        const bool bIsVirtualDesktop = BIT_SET(mouseData.usFlags, MOUSE_VIRTUAL_DESKTOP);

        const int32 width = GetSystemMetrics(bIsVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
        const int32 height = GetSystemMetrics(bIsVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

        bAbsPosUpdated = true;
        analogRawStates[AnalogStates::AbsMouseX] = (mouseData.lLastX / 65535.0f) * width;
        analogRawStates[AnalogStates::AbsMouseY] = (mouseData.lLastY / 65535.0f) * height;
    }
    else if (mouseData.usFlags == MOUSE_MOVE_RELATIVE)
    {
        bRelMoveUpdated = true;
        analogRawStates[AnalogStates::RelMouseX] += float(mouseData.lLastX);
        analogRawStates[AnalogStates::RelMouseY] += float(mouseData.lLastY);
    }

    return true;
}

bool WindowsMouseDevice::registerWindow(const GenericAppWindow *window) const
{
    RAWINPUTDEVICE mouseDevice;
    mouseDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
    mouseDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    mouseDevice.dwFlags = 0;
    mouseDevice.hwndTarget = (HWND)window->getWindowHandle();

    if (!RegisterRawInputDevices(&mouseDevice, 1, sizeof(decltype(mouseDevice))))
    {
        LOG_WARN("WindowsMouseDevice", "Failed registering mouse for window {}", window->getWindowName().getChar());
        return false;
    }

    return true;
}

void WindowsMouseDevice::pullProcessedInputs(Keys *keyStates, AnalogStates *analogStates)
{
    for (std::pair<Key const *const, KeyState> &keyState : keyStates->getKeyStates())
    {
        const EKeyCode keyCode = EKeyCode(keyState.first->keyCode);
        if (!Keys::isMouseKey(keyCode))
        {
            continue;
        }

        // Reset single frame states
        keyState.second.keyWentDown = keyState.second.keyWentUp = 0;

        auto rawKeyState = buttonRawStates.find(keyCode);
        if (rawKeyState != buttonRawStates.end())
        {
            // Not checking current pressed or release state before setting key went up/down to
            // allow OS's key press repeat after delay
            switch (rawKeyState->second)
            {
            case UP_STATE:
            {
                keyState.second.isPressed = 0;
                keyState.second.keyWentUp = 1;
                keyState.second.pressedTick = -1;
                break;
            }
            case DOWN_STATE:
            {
                keyState.second.pressedTick = keyState.second.isPressed ? keyState.second.pressedTick : Time::timeNow();
                keyState.second.isPressed = 1;
                keyState.second.keyWentDown = 1;
                break;
            }
            }
            rawKeyState->second = INVALID_STATE;
        }
        else
        {
            buttonRawStates.insert({ keyCode, INVALID_STATE });
        }
    }

    std::map<AnalogStates::EStates, InputAnalogState> &analogStatesMap = analogStates->getAnalogStates();
    /* Filling required analog states */
    /* Necessary as normal desktop do not publish absolute position */
    if (bRelMoveUpdated && !bAbsPosUpdated)
    {
        POINT cursorPos{ 0, 0 };
        if (GetCursorPos(&cursorPos))
        {
            analogRawStates[AnalogStates::AbsMouseX] = float(cursorPos.x);
            analogRawStates[AnalogStates::AbsMouseY] = float(cursorPos.y);
        }
    }
    if (bAbsPosUpdated && !bRelMoveUpdated)
    {
        analogRawStates[AnalogStates::RelMouseX]
            = analogRawStates[AnalogStates::AbsMouseX] - analogStatesMap[AnalogStates::AbsMouseX].currentValue;
        analogRawStates[AnalogStates::RelMouseY]
            = analogRawStates[AnalogStates::AbsMouseY] - analogStatesMap[AnalogStates::AbsMouseY].currentValue;
    }
    bAbsPosUpdated = false;
    bRelMoveUpdated = false;

    for (std::pair<const uint32, float> &rawAnalogState : analogRawStates)
    {
        InputAnalogState &outAnalogState = analogStatesMap[AnalogStates::EStates(rawAnalogState.first)];
        outAnalogState.startedThisFrame = (Math::isEqual(outAnalogState.currentValue, 0.0f) && rawAnalogState.second != 0.0f) ? 1 : 0;
        outAnalogState.stoppedThisFrame = (Math::isEqual(rawAnalogState.second, 0.0f) && outAnalogState.currentValue != 0.0f) ? 1 : 0;
        outAnalogState.acceleration = rawAnalogState.second - outAnalogState.currentValue;
        outAnalogState.currentValue = rawAnalogState.second;
        /* If absolute value we need it to be populated every frame but raw input arrives only when device sends messages */
        rawAnalogState.second = AnalogStates::isAbsoluteValue(AnalogStates::EStates(rawAnalogState.first)) ? rawAnalogState.second : 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// Keyboard device
//////////////////////////////////////////////////////////////////////////

bool WindowsKeyboardDevice::sendInRaw(const void *rawInput)
{
    const RAWINPUT *winRawInput = reinterpret_cast<const RAWINPUT *>(rawInput);
    if (winRawInput->header.dwType != RIM_TYPEKEYBOARD)
    {
        return false;
    }
    const uint8 keyState = (winRawInput->data.keyboard.Flags & RI_KEY_BREAK) == RI_KEY_BREAK ? UP_STATE : DOWN_STATE;
    /*
     * This is happening whenever multi-byte mapped keys are pressed
     * Currently we are not handling those keys properly
     */
    if (winRawInput->data.keyboard.VKey == 0xFF)
    {
        LOG_WARN(
            "WindowsKeyboardDevice", "Possible multibyte key that is not handled properly : {}, Flags : {}",
            winRawInput->data.keyboard.MakeCode, winRawInput->data.keyboard.Flags
        );
        return true;
    }
    // If E1 flag is there then it is pause/break
    if ((winRawInput->data.keyboard.Flags & RI_KEY_E1) == RI_KEY_E1)
    {
        rawKeyStates[EKeyCode::KEY_PAUSE] = keyState;
        return true;
    }

    EKeyCode keyCode = EKeyCode(winRawInput->data.keyboard.MakeCode);
    if ((winRawInput->data.keyboard.Flags & RI_KEY_E0) == RI_KEY_E0)
    {
        keyCode = EKeyCode(EKeyCode::E0_CODE | keyCode);
    }
    rawKeyStates[keyCode] = keyState;

    return true;
}

bool WindowsKeyboardDevice::registerWindow(const GenericAppWindow *window) const
{
    RAWINPUTDEVICE keyboardDevice;
    keyboardDevice.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    keyboardDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    keyboardDevice.dwFlags = 0;
    keyboardDevice.hwndTarget = (HWND)window->getWindowHandle();

    if (!RegisterRawInputDevices(&keyboardDevice, 1, sizeof(decltype(keyboardDevice))))
    {
        LOG_WARN("WindowsKeyboardDevice", "Failed registering keyboard for window {}", window->getWindowName().getChar());
        return false;
    }
    return true;
}

void WindowsKeyboardDevice::pullProcessedInputs(Keys *keyStates, AnalogStates *analogStates)
{
    for (std::pair<Key const *const, KeyState> &keyState : keyStates->getKeyStates())
    {
        const EKeyCode keyCode = EKeyCode(keyState.first->keyCode);
        if (!Keys::isKeyboardKey(keyCode))
        {
            continue;
        }

        // Reset single frame states
        keyState.second.keyWentDown = keyState.second.keyWentUp = 0;

        auto rawKeyState = rawKeyStates.find(keyCode);
        if (rawKeyState != rawKeyStates.end())
        {
            // Not checking current pressed or release state before setting key went up/down to
            // allow OS's key press repeat after delay
            switch (rawKeyState->second)
            {
            case UP_STATE:
            {
                keyState.second.isPressed = 0;
                keyState.second.keyWentUp = 1;
                keyState.second.pressedTick = -1;
                break;
            }
            case DOWN_STATE:
            {
                keyState.second.pressedTick = keyState.second.isPressed ? keyState.second.pressedTick : Time::timeNow();
                keyState.second.isPressed = 1;
                keyState.second.keyWentDown = 1;
                break;
            }
            }
            rawKeyState->second = INVALID_STATE;
        }
        else
        {
            rawKeyStates.insert({ keyCode, INVALID_STATE });
        }
    }

    // Filling direct accessible analog states
    analogRawStates[AnalogStates::CapsLock] = GetKeyState(VK_CAPITAL) & 0x0001;
    analogRawStates[AnalogStates::NumLock] = GetKeyState(VK_NUMLOCK) & 0x0001;
    analogRawStates[AnalogStates::ScrollLock] = GetKeyState(VK_SCROLL) & 0x0001;

    std::map<AnalogStates::EStates, InputAnalogState> &analogStatesMap = analogStates->getAnalogStates();
    for (std::pair<const uint32, int8> &rawAnalogState : analogRawStates)
    {
        InputAnalogState &outAnalogState = analogStatesMap[AnalogStates::EStates(rawAnalogState.first)];
        outAnalogState.startedThisFrame = (!(outAnalogState.currentValue > 0.0f) && rawAnalogState.second != 0) ? 1 : 0;
        outAnalogState.stoppedThisFrame = (rawAnalogState.second == 0 && outAnalogState.currentValue > 0.0f) ? 1 : 0;
        outAnalogState.acceleration = rawAnalogState.second - outAnalogState.currentValue;
        outAnalogState.currentValue = rawAnalogState.second;
        /* If absolute value we need it to be populated every frame but raw input arrives only when device sends messages */
        rawAnalogState.second = AnalogStates::isAbsoluteValue(AnalogStates::EStates(rawAnalogState.first)) ? rawAnalogState.second : 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// Gamepad device
//////////////////////////////////////////////////////////////////////////

bool WindowsGamepadDevice::registerWindow(const GenericAppWindow *window) const
{
    RAWINPUTDEVICE gamepadDevices[] = {
        {.usUsagePage = HID_USAGE_PAGE_GENERIC,
         .usUsage = HID_USAGE_GENERIC_GAMEPAD,
         .dwFlags = 0,
         .hwndTarget = (HWND)window->getWindowHandle()},
        {.usUsagePage = HID_USAGE_PAGE_GENERIC,
         .usUsage = HID_USAGE_GENERIC_JOYSTICK,
         .dwFlags = 0,
         .hwndTarget = (HWND)window->getWindowHandle()}
    };

    if (!RegisterRawInputDevices(gamepadDevices, ARRAY_LENGTH(gamepadDevices), sizeof(decltype(gamepadDevices[0]))))
    {
        LOG_WARN("WindowsGamepadDevice", "Failed registering gamepads for window {}", window->getWindowName().getChar());
        return false;
    }

    return true;
}

bool WindowsGamepadDevice::sendInRaw(const void *rawInput)
{
    const RAWINPUT *winRawInput = reinterpret_cast<const RAWINPUT *>(rawInput);
    if (winRawInput->header.dwType != RIM_TYPEHID)
    {
        return false;
    }
    uint32 bytesCount = sizeof(RID_DEVICE_INFO);
    RID_DEVICE_INFO devInfo;
    if (::GetRawInputDeviceInfo(winRawInput->header.hDevice, RIDI_DEVICEINFO, &devInfo, &bytesCount) == -1
        || (devInfo.hid.usUsage != HID_USAGE_GENERIC_GAMEPAD && devInfo.hid.usUsage != HID_USAGE_GENERIC_JOYSTICK))
    {
        return false;
    }

    // TODO(Jeslas) : Process gamepad input

    return true;
}
void WindowsGamepadDevice::pullProcessedInputs(Keys *keyStates, AnalogStates *analogStates)
{
    // TODO(Jeslas) : Process gamepad input
    CompilerHacks::ignoreUnused(keyStates);
    CompilerHacks::ignoreUnused(analogStates);
}