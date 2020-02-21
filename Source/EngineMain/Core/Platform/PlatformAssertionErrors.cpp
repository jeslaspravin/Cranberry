#include "PlatformAssertionErrors.h"



#if _WIN32

#include "Windows/ErrorsAsserts/WindowsErrorHandler.h"

#elif __unix__

static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif

UnexpectedErrorHandler* UnexpectedErrorHandler::getHandler()
{
    return PlatformUnexpectedErrorHandler::getHandler();
}
