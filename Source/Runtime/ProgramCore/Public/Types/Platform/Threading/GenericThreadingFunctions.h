/*!
 * \file GenericThreadingFunctions.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"

template <typename PlatformType>
class GenericThreadingFunctions
{
public:
    static void* createProcess(const String& applicationPath, const String& cmdLine, const String& environment, const String& workingDirectory)
    {
        return PlatformType::createProcess(applicationPath, cmdLine, environment, workingDirectory);
    }
};