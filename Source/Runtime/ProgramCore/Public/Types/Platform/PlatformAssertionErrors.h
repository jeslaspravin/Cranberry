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
    static UnexpectedErrorHandler* getHandler();

    virtual void registerFilter() = 0;
    virtual void unregisterFilter() = 0;
    virtual void dumpCallStack(bool bShouldCrashApp) = 0;
};

#ifndef debugAssert
#if _DEBUG
#include <assert.h>
#define debugAssert(Expr)\
do{\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed "#Expr,__func__);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack(false);}\
    assert((Expr));\
}while(0)
#else
#define debugAssert(Expr)
#endif// #define debugAssert(Expr)
#endif// #ifndef debugAssert

#ifndef fatalAssert
#define fatalAssert(Expr,Message, ...)\
do{\
if(!(Expr)){\
    Logger::error("DebugAssertion", "%s() : Assert expression failed ["#Expr"] "#Message,__func__, __VA_ARGS__);\
    UnexpectedErrorHandler::getHandler()->dumpCallStack(true);}\
}while(0)
#endif