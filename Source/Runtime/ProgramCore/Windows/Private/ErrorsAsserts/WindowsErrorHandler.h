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

class WindowsUnexpectedErrorHandler : public UnexpectedErrorHandler
{
private:
    typedef long (*PreviousFilterFunc)(struct _EXCEPTION_POINTERS *ExceptionInfo);
    PreviousFilterFunc previousFilter;

    void dumpStack(struct _CONTEXT *context, bool bCloseApp) const;

public:
    static WindowsUnexpectedErrorHandler *getHandler()
    {
        static WindowsUnexpectedErrorHandler handler;
        return &handler;
    }

    static long handlerFilter(struct _EXCEPTION_POINTERS *exp);

    /* UnexpectedErrorHandler Implementation */
    void registerPlatformFilters() override;
    void unregisterPlatformFilters() const override;
    void dumpCallStack(bool bShouldCrashApp) const override;
    void debugBreak() const override;
    /* Ends */
};

typedef WindowsUnexpectedErrorHandler PlatformUnexpectedErrorHandler;
