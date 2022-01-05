/*!
 * \file PlatformLFS.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS
#include "LFS/File/WindowsFile.h"
#include "LFS/WindowsFileSystemFunctions.h"
#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif
#include "File/GenericFileHandle.h"

typedef LFS::FileSystemFunctions FileSystemFunctions;
typedef LFS::PlatformFile PlatformFile;