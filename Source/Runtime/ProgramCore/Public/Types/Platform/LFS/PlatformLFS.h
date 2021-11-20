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