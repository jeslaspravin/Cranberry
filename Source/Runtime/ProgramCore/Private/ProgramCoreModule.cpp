/*!
 * \file ProgramCoreModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Modules/ModuleManager.h"
#include "Types/ProgramCoreDelegates.h"

class ProgramCoreModule : public ModuleNoImpl
{};

DECLARE_MODULE(ProgramCore, ProgramCoreModule)

//////////////////////////////////////////////////////////////////////////
/// ProgramCoreDelegates implementations
//////////////////////////////////////////////////////////////////////////

ProgramCoreDelegates::ApplicationCrashEvent ProgramCoreDelegates::onApplicationCrash;