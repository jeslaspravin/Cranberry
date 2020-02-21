#pragma once
#include "../../PlatformAssertionErrors.h"

class WindowsUnexpectedErrorHandler : public UnexpectedErrorHandler
{
private:
    typedef long (*PreviousFilterFunc)(struct _EXCEPTION_POINTERS* ExceptionInfo);
    PreviousFilterFunc previousFilter;

    void dumpStack(struct _CONTEXT* context);
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
    void dumpCallStack() override;
    /* Ends */
};


typedef WindowsUnexpectedErrorHandler PlatformUnexpectedErrorHandler;
