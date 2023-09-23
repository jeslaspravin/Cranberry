/*!
 * \file Directories.cs
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

﻿using System.Runtime.InteropServices;
using System.IO;

namespace GameBuilder.BuildFileUtils
{
    static class Directories
    {
        public static string ENGINE_ROOT = "";
        // Build paths
        public static string BUILD_ROOT = "";
        public static string ENGINE_BINARIES_ROOT = "";
        public static string TOOLS_BUILD_ROOT = "";
        public static string TOOLS_BINARIES_ROOT = "";
        public static string BUILDER_EXE = "";

        // Names
        public static string APP_NAME = "";
        public static string BUILD_CONFIG_NAME = "";
        // Ends with path separator if any prefix else empty
        public static string BUILD_CONFIG_PATH_PREFIX = "";

        static Directories()
        {
            TOOLS_BINARIES_ROOT = TOOLS_BUILD_ROOT = Directory.GetCurrentDirectory();
            if(platformHasBuildConfigPrefix())
            {
                BUILD_CONFIG_NAME = Path.GetFileNameWithoutExtension(TOOLS_BINARIES_ROOT);
                BUILD_CONFIG_PATH_PREFIX = BUILD_CONFIG_NAME + Path.PathSeparator;
                // Setting paths without config appended
                TOOLS_BUILD_ROOT = Directory.GetParent(TOOLS_BINARIES_ROOT).FullName;
            }
            BUILD_ROOT = Directory.GetParent(TOOLS_BUILD_ROOT).FullName;
            ENGINE_ROOT = Directory.GetParent(BUILD_ROOT).FullName;
            ENGINE_BINARIES_ROOT = BUILD_ROOT + BUILD_CONFIG_PATH_PREFIX;

            APP_NAME = System.Reflection.Assembly.GetExecutingAssembly().GetName().Name;
            BUILDER_EXE = Path.Combine(TOOLS_BINARIES_ROOT, APP_NAME + ".exe");
        }

        private static bool platformHasBuildConfigPrefix()
        {
            return RuntimeInformation.IsOSPlatform(OSPlatform.Windows) || RuntimeInformation.IsOSPlatform(OSPlatform.OSX);
        }
    }
}
