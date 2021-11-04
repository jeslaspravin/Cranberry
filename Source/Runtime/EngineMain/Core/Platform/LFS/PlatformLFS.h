#pragma once
#if _WIN32
#include "../Windows/LFS/File/WindowsFile.h"
#include "../Windows/LFS/WindowsFileSystemFunctions.h"
#elif __unix__
static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif
#include "File/GenericFileHandle.h"

typedef LFS::FileSystemFunctions FileSystemFunctions;
typedef LFS::PlatformFile PlatformFile;