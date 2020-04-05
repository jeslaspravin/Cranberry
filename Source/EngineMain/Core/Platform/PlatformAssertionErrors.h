#pragma once

class UnexpectedErrorHandler
{
public:
    static UnexpectedErrorHandler* getHandler();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() = 0;
    virtual void dumpCallStack(bool bShouldCrashEngine) = 0;
};

#ifndef debugAssert
#if _DEBUG
#include <assert.h>
#define debugAssert(Expr)\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr,__func__);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack(false);}\
    assert((Expr));
#else
#define debugAssert(Expr)\
if((Expr)){}
#endif// #define debugAssert(Expr)
#endif// #ifndef debugAssert

#ifndef fatalAssert
#define fatalAssert(Expr,Message)\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr"[%s]",__func__,Message);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack(true);}
#endif