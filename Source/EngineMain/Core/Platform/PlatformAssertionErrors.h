#pragma once

class UnexpectedErrorHandler
{
public:
    static UnexpectedErrorHandler* getHandler();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() = 0;
    virtual void dumpCallStack() = 0;
};

#ifndef debugAssert
#if _DEBUG
#define debugAssert(Expr)\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr,__func__);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack();}
#else
#define debugAssert(Expr)\
if((Expr)){}
#endif// #define debugAssert(Expr)
#endif// #ifndef debugAssert

#ifndef fatalAssert
#define fatalAssert(Expr,Message)\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr"[%s]",__func__,Message);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack();}
#endif