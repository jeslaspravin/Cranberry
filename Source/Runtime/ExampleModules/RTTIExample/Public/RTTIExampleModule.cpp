/*!
 * \file RTTIExampleModule.cpp
 *
 * \author Jeslas Pravin
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Modules/ModuleManager.h"

class RTTIExampleModule : public ModuleNoImpl
{};

DECLARE_MODULE(RTTIExample, RTTIExampleModule)