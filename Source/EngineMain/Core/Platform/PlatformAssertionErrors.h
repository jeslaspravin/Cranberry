#pragma once

class UnexpectedErrorHandler
{
public:
    static UnexpectedErrorHandler* getHandler();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() = 0;
    virtual void dumpCallStack() = 0;
};