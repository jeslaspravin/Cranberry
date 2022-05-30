/*!
 * \file PlatformAssertionErrors.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/PlatformAssertionErrors.h"

#if PLATFORM_WINDOWS

#include "ErrorsAsserts/WindowsErrorHandler.h"

#elif PLATFORM_LINUX
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif

UnexpectedErrorHandler *UnexpectedErrorHandler::getHandler() { return PlatformUnexpectedErrorHandler::getHandler(); }
