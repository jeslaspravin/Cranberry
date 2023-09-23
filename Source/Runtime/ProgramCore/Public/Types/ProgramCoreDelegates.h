/*!
 * \file ProgramCoreDelegates.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Delegates/Delegate.h"
#include "ProgramCoreExports.h"

class UnexpectedErrorHandler;

class PROGRAMCORE_EXPORT ProgramCoreDelegates
{
private:
    ProgramCoreDelegates() = default;

public:
    using ApplicationCrashEvent = Event<UnexpectedErrorHandler>;
    static ApplicationCrashEvent onApplicationCrash;
};
