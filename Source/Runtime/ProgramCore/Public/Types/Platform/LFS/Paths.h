/*!
 * \file Paths.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"

class String;

class PROGRAMCORE_EXPORT Paths
{
private:
    Paths() = default;

public:
    // Without extension
    static String applicationDirectory(String &appName, String *extension = nullptr);
    static String applicationDirectory();
    // Without extension
    static String applicationName();

    static String engineRoot();
};
