#pragma once

#include "../Logger/Logger.h"

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
do{\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed %s",#Expr,__func__);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack(false);}\
    assert((Expr));\
}while(0)
#else
#define debugAssert(Expr)
#endif// #define debugAssert(Expr)
#endif// #ifndef debugAssert

#ifndef fatalAssert
#define fatalAssert(Expr,Message)\
do{\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed %s [%s]",__func__, #Expr, Message);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack(true);}\
}while(0)
#endif