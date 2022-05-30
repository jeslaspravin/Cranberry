/*!
 * \file PlatformInputTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS

#include "InputSystem/WindowsKeyCodes.h"
#include "InputSystem/WindowsKeyboardDevice.h"
#include "InputSystem/WindowsMouseDevice.h"
#include "InputSystem/WindowsRawInputBuffer.h"

#elif PLATFORM_LINUX
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif
#include "MakeBreakKeyCodes.h"

// using EKeyCode = KeyCode::EKeyCode;
using EKeyCode = MakeBreakKeyCodes::EKeyCode;

using MouseDevice = InputDevices::MouseDevice;
using KeyboardDevice = InputDevices::KeyboardDevice;

using RawInputBuffer = Input::RawInputBuffer;