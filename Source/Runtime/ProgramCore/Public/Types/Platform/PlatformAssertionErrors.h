/*!
 * \file PlatformAssertionErrors.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Logger/Logger.h"
#include "Types/CompilerDefines.h"
#include "ProgramCoreExports.h"

class PROGRAMCORE_EXPORT UnexpectedErrorHandler
{
public:
    static UnexpectedErrorHandler *getHandler();
    /**
     * Deliberately crashes the application after trying to close core systems. Tries breaking the application before that
     */
    static void crashApplication();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() const = 0;
    virtual void dumpCallStack(bool bShouldCrashApp) const = 0;
    /**
     * Calls debugger only if present else does nothing
     */
    virtual void debugBreak() const = 0;
};

#define LOG_ASSERTION_FORMATTED(Expr, Category, Message, ...) LOG_ERROR(Category, "Assert expression failed [" #Expr "] " Message, __VA_ARGS__)
#define LOG_ASSERTION(Expr, Category) LOG_ERROR(Category, "Assert expression failed " #Expr)

#if DEBUG_VALIDATIONS
#include <assert.h>

#ifndef debugAssert
#define debugAssert(Expr)                                                                                                                      \
    do                                                                                                                                         \
    {                                                                                                                                          \
        if (!(Expr))                                                                                                                           \
        {                                                                                                                                      \
            LOG_ASSERTION(Expr, "DebugAssertion");                                                                                             \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(false);                                                                        \
        }                                                                                                                                      \
        /* Using assert macro to make use of assert window to crash or debug */                                                                \
        assert((Expr));                                                                                                                        \
    }                                                                                                                                          \
    while (0)
#endif // #ifndef debugAssert

// Debug assert or slow assert
#ifndef debugAssertf
#define debugAssertf(Expr, Message, ...)                                                                                                       \
    do                                                                                                                                         \
    {                                                                                                                                          \
        if (!(Expr))                                                                                                                           \
        {                                                                                                                                      \
            LOG_ASSERTION_FORMATTED(Expr, "DebugAssertion", Message, __VA_ARGS__);                                                             \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(false);                                                                        \
        }                                                                                                                                      \
        /* Using assert macro to make use of assert window to crash or debug */                                                                \
        assert((Expr));                                                                                                                        \
    }                                                                                                                                          \
    while (0)
#endif // #ifndef debugAssertf

#else // DEBUG_VALIDATIONS
#define debugAssert(Expr)
#define debugAssertf(Expr, Message, ...)
#endif // DEBUG_VALIDATIONS

#ifndef fatalAssert
#define fatalAssert(Expr)                                                                                                                      \
    do                                                                                                                                         \
    {                                                                                                                                          \
        if (!(Expr))                                                                                                                           \
        {                                                                                                                                      \
            LOG_ASSERTION(Expr, "FatalAssertion");                                                                                             \
            UnexpectedErrorHandler::getHandler()->debugBreak();                                                                                \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(true);                                                                         \
        }                                                                                                                                      \
    }                                                                                                                                          \
    while (0)
#endif
#ifndef fatalAssertf
#define fatalAssertf(Expr, Message, ...)                                                                                                       \
    do                                                                                                                                         \
    {                                                                                                                                          \
        if (!(Expr))                                                                                                                           \
        {                                                                                                                                      \
            LOG_ASSERTION_FORMATTED(Expr, "FatalAssertion", Message, __VA_ARGS__);                                                             \
            UnexpectedErrorHandler::getHandler()->debugBreak();                                                                                \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(true);                                                                         \
        }                                                                                                                                      \
    }                                                                                                                                          \
    while (0)
#endif

#define ALERT_internal(Expr, DebugBreakCaller)                                                                                                  \
    if (!(Expr))                                                                                                                               \
    {                                                                                                                                          \
        LOG_ASSERTION(Expr, "AlertAssertion");                                                                                                 \
        UnexpectedErrorHandler::getHandler()->dumpCallStack(false);                                                                            \
        DebugBreakCaller(UnexpectedErrorHandler::getHandler()->debugBreak());                                                                   \
    }

#define ALERT_FORMATTED_internal(Expr, DebugBreakCaller, Message, ...)                                                                          \
    if (!(Expr))                                                                                                                               \
    {                                                                                                                                          \
        LOG_ASSERTION_FORMATTED(Expr, "AlertAssertion", Message, __VA_ARGS__);                                                                 \
        UnexpectedErrorHandler::getHandler()->dumpCallStack(false);                                                                            \
        DebugBreakCaller(UnexpectedErrorHandler::getHandler()->debugBreak());                                                                   \
    }

#ifndef alertAlways
#define alertAlways(Expr)                                                                                                                      \
    do                                                                                                                                         \
    {                                                                                                                                          \
        ALERT_internal(Expr, EXPAND_ARGS)                                                                                                      \
    }                                                                                                                                          \
    while (0)
#endif
#ifndef alertAlwaysf
#define alertAlwaysf(Expr, Message, ...)                                                                                                       \
    do                                                                                                                                         \
    {                                                                                                                                          \
        ALERT_FORMATTED_internal(Expr, EXPAND_ARGS, Message, __VA_ARGS__)                                                                      \
    }                                                                                                                                          \
    while (0)
#endif

#ifndef alertOnce
#define alertOnce(Expr)                                                                                                                        \
    do                                                                                                                                         \
    {                                                                                                                                          \
        /* We have to dump any way */                                                                                                          \
        ALERT_internal(Expr, IGNORE_ARGS);                                                                                                     \
        CALL_ONCE(                                                                                                                             \
            [&]()                                                                                                                              \
            {                                                                                                                                  \
                if (!(Expr))                                                                                                                   \
                {                                                                                                                              \
                    UnexpectedErrorHandler::getHandler()->debugBreak();                                                                        \
                }                                                                                                                              \
            }                                                                                                                                  \
        );                                                                                                                                     \
    }                                                                                                                                          \
    while (0)
#endif
#ifndef alertOncef
#define alertOncef(Expr, Message, ...)                                                                                                         \
    do                                                                                                                                         \
    {                                                                                                                                          \
        /* We have to dump any way */                                                                                                          \
        ALERT_FORMATTED_internal(Expr, IGNORE_ARGS, Message, __VA_ARGS__);                                                                     \
        CALL_ONCE(                                                                                                                             \
            [&]()                                                                                                                              \
            {                                                                                                                                  \
                if (!(Expr))                                                                                                                   \
                {                                                                                                                              \
                    UnexpectedErrorHandler::getHandler()->debugBreak();                                                                        \
                }                                                                                                                              \
            }                                                                                                                                  \
        );                                                                                                                                     \
    }                                                                                                                                          \
    while (0)
#endif