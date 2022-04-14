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
#include "ProgramCoreExports.h"

class PROGRAMCORE_EXPORT UnexpectedErrorHandler
{
public:
    static UnexpectedErrorHandler *getHandler();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() const = 0;
    virtual void dumpCallStack(bool bShouldCrashApp) const = 0;
    /**
     * Calls debugger only if present else does nothing
     */
    virtual void debugBreak() const = 0;
};

// Debug assert or slow assert
#ifndef debugAssert
#if DEBUG_VALIDATIONS
#include <assert.h>
#define debugAssert(Expr)                                                                               \
    do                                                                                                  \
    {                                                                                                   \
        if (!(Expr))                                                                                    \
        {                                                                                               \
            LOG_ERROR("DebugAssertion", "%s() : Assert expression failed " #Expr, __func__);            \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(false);                                 \
        }                                                                                               \
        /* Using assert macro to make use of assert window to crash or debug */                         \
        assert((Expr));                                                                                 \
    }                                                                                                   \
    while (0)
#else
#define debugAssert(Expr)
#endif // #define debugAssert(Expr)
#endif // #ifndef debugAssert

#ifndef fatalAssert
#define fatalAssert(Expr, Message, ...)                                                                 \
    do                                                                                                  \
    {                                                                                                   \
        if (!(Expr))                                                                                    \
        {                                                                                               \
            LOG_ERROR("FatalAssertion", "%s() : Assert expression failed [" #Expr "] " #Message,        \
                __func__, __VA_ARGS__);                                                                 \
            UnexpectedErrorHandler::getHandler()->debugBreak();                                         \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(true);                                  \
        }                                                                                               \
    }                                                                                                   \
    while (0)
#endif

#ifndef alertIf
#define alertIf(Expr, Message, ...)                                                                     \
    do                                                                                                  \
    {                                                                                                   \
        if (!(Expr))                                                                                    \
        {                                                                                               \
            LOG_ERROR("DebugAssertion", "%s() : Signalling failure [" #Expr "] " #Message, __func__,    \
                __VA_ARGS__);                                                                           \
            UnexpectedErrorHandler::getHandler()->dumpCallStack(false);                                 \
            UnexpectedErrorHandler::getHandler()->debugBreak();                                         \
        }                                                                                               \
    }                                                                                                   \
    while (0)
#endif