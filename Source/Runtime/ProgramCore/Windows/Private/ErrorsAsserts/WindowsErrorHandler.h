/*!
 * \file WindowsErrorHandler.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/PlatformAssertionErrors.h"

struct _EXCEPTION_POINTERS;
struct _CONTEXT;

class WindowsUnexpectedErrorHandler : public UnexpectedErrorHandler
{
public:
    static WindowsUnexpectedErrorHandler *getHandler()
    {
        static WindowsUnexpectedErrorHandler handler;
        return &handler;
    }

    static long handlerFilter(_EXCEPTION_POINTERS *exp) noexcept;

    /* UnexpectedErrorHandler Implementation */
    void registerPlatformFilters() override;
    void unregisterPlatformFilters() const override;
    void dumpCallStack(bool bShouldCrashApp) const override;
    void debugBreak() const override;
    /* Ends */
private:
    typedef long (*PreviousFilterFunc)(_EXCEPTION_POINTERS *ExceptionInfo);
    PreviousFilterFunc previousFilter;

    _CONTEXT *getCurrentExceptionCntxt() const;
    void dumpStack(_CONTEXT *context, bool bCloseApp) const;
};

typedef WindowsUnexpectedErrorHandler PlatformUnexpectedErrorHandler;
