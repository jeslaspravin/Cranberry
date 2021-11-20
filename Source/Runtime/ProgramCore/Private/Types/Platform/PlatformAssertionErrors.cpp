#include "Types/Platform/PlatformAssertionErrors.h"


#if PLATFORM_WINDOWS

#include "ErrorsAsserts/WindowsErrorHandler.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif

UnexpectedErrorHandler* UnexpectedErrorHandler::getHandler()
{
    return PlatformUnexpectedErrorHandler::getHandler();
}
