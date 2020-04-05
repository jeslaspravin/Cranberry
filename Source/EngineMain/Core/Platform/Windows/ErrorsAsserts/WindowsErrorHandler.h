#pragma once
#include "../../PlatformAssertionErrors.h"

class WindowsUnexpectedErrorHandler : public UnexpectedErrorHandler
{
private:
    typedef long (*PreviousFilterFunc)(struct _EXCEPTION_POINTERS* ExceptionInfo);
    PreviousFilterFunc previousFilter;

    void dumpStack(struct _CONTEXT* context, bool bCloseEngine);
public:
    
    static WindowsUnexpectedErrorHandler* getHandler()
    {
        static WindowsUnexpectedErrorHandler handler;
        return &handler;
    }

    static long handlerFilter(struct _EXCEPTION_POINTERS* exp);

    /* UnexpectedErrorHandler Implementation */
    void registerFilter() override;
    void unregisterFilter() override;
    void dumpCallStack(bool bShouldCrashEngine) override;
    /* Ends */
};


typedef WindowsUnexpectedErrorHandler PlatformUnexpectedErrorHandler;
