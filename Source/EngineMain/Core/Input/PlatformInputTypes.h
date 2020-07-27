#pragma once

#if _WIN32

#include "Windows/WindowsKeyCodes.h"
#include "Windows/WindowsKeyboardDevice.h"
#include "Windows/WindowsMouseDevice.h"
#include "Windows/WindowsRawInputBuffer.h"

#elif __unix__

static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif
#include "MakeBreakKeyCodes.h"

//using EKeyCode = KeyCode::EKeyCode;
using EKeyCode = MakeBreakKeyCodes::EKeyCode;

using MouseDevice = InputDevices::MouseDevice;
using KeyboardDevice = InputDevices::KeyboardDevice;

using RawInputBuffer = Input::RawInputBuffer;