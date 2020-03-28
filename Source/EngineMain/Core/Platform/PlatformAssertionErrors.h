#pragma once

class UnexpectedErrorHandler
{
public:
    static UnexpectedErrorHandler* getHandler();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() = 0;
    virtual void dumpCallStack() = 0;
};

#if _DEBUG
#ifndef dbgAssert
#define dbgAssert(Expr)\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr,__func__);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack();}
#endif
#else
#ifndef dbgAssert
#define dbgAssert(Expr)\
if((Expr)){}
#endif
#endif

#ifndef fatalAssert
#define fatalAssert(Expr,Message)\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr"[%s]",__func__,Message);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack();}
#endif