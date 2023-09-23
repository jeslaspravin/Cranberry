/*!
 * \file Paths.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "Types/CoreTypes.h"

class String;

class PROGRAMCORE_EXPORT Paths
{
private:
    Paths() = default;

public:
    // Without extension
    static String applicationDirectory(String &appName, String *extension = nullptr);
    static String applicationDirectory();
    static String savedDirectory();
    static String contentDirectory();
    // Without extension
    static const TChar *applicationName();

    static const TChar *engineRoot();
    /**
     * Since Runtime, Tools, Editor exists in EngineRoot/Runtime/../[Tools|Editor] We can determine the EngineRoot and go into other library
     * locations from there
     */
    static const TChar *engineRuntimeRoot();
};
