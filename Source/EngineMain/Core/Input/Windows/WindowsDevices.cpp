#include "WindowsKeyboardDevice.h"
#include "WindowsMouseDevice.h"
#include "../../Platform/Windows/WindowsCommonHeaders.h"
#include "../../Platform/Windows/WindowsAppWindow.h"
#include "../../Logger/Logger.h"
#include "../../Math/Math.h"
#include "../Keys.h"
#include "../PlatformInputTypes.h"

#include <hidusage.h>

//////////////////////////////////////////////////////////////////////////
// Mouse device
//////////////////////////////////////////////////////////////////////////

WindowsMouseDevice::WindowsMouseDevice()
{
    analogRawStates[AnalogStates::RelMouseX] = analogRawStates[AnalogStates::RelMouseY] = 0;
    analogRawStates[AnalogStates::ScrollWheelX] = analogRawStates[AnalogStates::ScrollWheelY] = 0;
}

bool WindowsMouseDevice::sendInRaw(const void* rawInput)
{
    const RAWINPUT* winRawInput = reinterpret_cast<const RAWINPUT*>(rawInput);
    if (winRawInput->header.dwType != RIM_TYPEMOUSE)
    {
        return false;
    }

    const RAWMOUSE& mouseData = winRawInput->data.mouse;
    if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_1_UP)) != 0)// LMB
    {
        buttonRawStates[EKeyCode::MOUSE_LEFT] = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) == RI_MOUSE_BUTTON_1_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_2_DOWN | RI_MOUSE_BUTTON_2_UP)) != 0)// RMB
    {
        buttonRawStates[EKeyCode::MOUSE_RIGHT] = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) == RI_MOUSE_BUTTON_2_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_3_DOWN | RI_MOUSE_BUTTON_3_UP)) != 0)// MMB
    {
        buttonRawStates[EKeyCode::MOUSE_MID] = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) == RI_MOUSE_BUTTON_3_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_4_UP)) != 0)// X1MB
    {
        buttonRawStates[EKeyCode::MOUSE_X1] = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) == RI_MOUSE_BUTTON_5_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & (RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_5_UP)) != 0)// X2MB
    {
        buttonRawStates[EKeyCode::MOUSE_X2] = (mouseData.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) == RI_MOUSE_BUTTON_5_DOWN ? DOWN_STATE : UP_STATE;
    }
    else if ((mouseData.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL)
    {
        analogRawStates[AnalogStates::ScrollWheelY] = float(int16(mouseData.usButtonData)) / WHEEL_DELTA;
    }
    else if ((mouseData.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL)
    {
        analogRawStates[AnalogStates::ScrollWheelX] = float(int16(mouseData.usButtonData)) / WHEEL_DELTA;
    }

    if (!(mouseData.usFlags & MOUSE_MOVE_ABSOLUTE))
    {
        analogRawStates[AnalogStates::RelMouseX] += float(mouseData.lLastX);
        analogRawStates[AnalogStates::RelMouseY] += float(mouseData.lLastY);
    }
    else if ((mouseData.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP)
    {
        Logger::warn("WindowMouseDevice", "%s : Virtual desktop setup is not supported", __func__);
    }

    // TODO(Jeslas) : Change absolute position retrieval to some other better way(using this as this is only way i found that gives screen coordinate even when mouse is out of client rect)
    MSG absPosMsg;
    if (PeekMessage(&absPosMsg, nullptr, 0, 0, PM_NOREMOVE))
    {
        analogRawStates[AnalogStates::AbsMouseX] = float(absPosMsg.pt.x);
        analogRawStates[AnalogStates::AbsMouseY] = float(absPosMsg.pt.y);
    }

    return true;
}

bool WindowsMouseDevice::registerWindow(const GenericAppWindow* window) const
{
    RAWINPUTDEVICE mouseDevice;
    mouseDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
    mouseDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    mouseDevice.dwFlags = 0;
    mouseDevice.hwndTarget = static_cast<const WindowsAppWindow*>(window)->getWindowHandle();

    if (!RegisterRawInputDevices(&mouseDevice, 1, sizeof(decltype(mouseDevice))))
    {
        Logger::warn("WindowsMouseDevice", "%s : Failed registering mouse for window %s", __func__, window->getWindowName().getChar());
        return false;
    }
    return true;
}

void WindowsMouseDevice::pullProcessedInputs(Keys* keyStates, AnalogStates* analogStates)
{
    for (std::pair<Key const* const, KeyState>& keyState : keyStates->getKeyStates())
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
            switch (rawKeyState->second)
            {
            case UP_STATE:
            {
                keyState.second.isPressed = 0;
                keyState.second.keyWentUp = 1;
                break;
            }
            case DOWN_STATE:
            {
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

    std::map<AnalogStates::EStates, InputAnalogState>& analogStatesMap = analogStates->getAnalogStates();
    for (std::pair<const uint32, float>& rawAnalogState : analogRawStates)
    {
        InputAnalogState& outAnalogState = analogStatesMap[AnalogStates::EStates(rawAnalogState.first)];

        outAnalogState.startedThisFrame = (Math::isEqual(outAnalogState.currentValue, 0.0f) && rawAnalogState.second != 0.0f)
            ? 1 : 0;
        outAnalogState.stoppedThisFrame = (Math::isEqual(rawAnalogState.second, 0.0f) && outAnalogState.currentValue != 0.0f)
            ? 1 : 0;
        outAnalogState.acceleration = rawAnalogState.second - outAnalogState.currentValue;
        outAnalogState.currentValue = rawAnalogState.second;

        rawAnalogState.second = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// Keyboard device
//////////////////////////////////////////////////////////////////////////

bool WindowsKeyboardDevice::sendInRaw(const void* rawInput)
{
    const RAWINPUT* winRawInput = reinterpret_cast<const RAWINPUT*>(rawInput);
    if (winRawInput->header.dwType != RIM_TYPEKEYBOARD)
    {
        return false;
    }
    const uint8 keyState = (winRawInput->data.keyboard.Flags & RI_KEY_BREAK) == RI_KEY_BREAK ? UP_STATE : DOWN_STATE;

    EKeyCode keyCode = EKeyCode(winRawInput->data.keyboard.MakeCode);
    if ((winRawInput->data.keyboard.Flags & RI_KEY_E0) == RI_KEY_E0)
    {
        keyCode = EKeyCode(EKeyCode::E0_CODE | keyCode);
    }
    rawKeyStates[keyCode] = keyState;

    return true;
}

bool WindowsKeyboardDevice::registerWindow(const GenericAppWindow* window) const
{
    RAWINPUTDEVICE keyboardDevice;
    keyboardDevice.usUsage = HID_USAGE_GENERIC_KEYBOARD;
    keyboardDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
    keyboardDevice.dwFlags = 0;
    keyboardDevice.hwndTarget = static_cast<const WindowsAppWindow*>(window)->getWindowHandle();

    if (!RegisterRawInputDevices(&keyboardDevice, 1, sizeof(decltype(keyboardDevice))))
    {
        Logger::warn("WindowsKeyboardDevice", "%s : Failed registering keyboard for window %s", __func__, window->getWindowName().getChar());
        return false;
    }
    return true;
}

void WindowsKeyboardDevice::pullProcessedInputs(Keys* keyStates, AnalogStates* analogStates)
{
    for (std::pair<Key const* const, KeyState>& keyState : keyStates->getKeyStates())
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
            switch (rawKeyState->second)
            {
            case UP_STATE:
            {
                keyState.second.isPressed = 0;
                keyState.second.keyWentUp = 1;
                break;
            }
            case DOWN_STATE:
            {
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
}
