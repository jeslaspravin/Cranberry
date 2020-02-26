#include "WindowsFileSystemFunctions.h"
#include "File/WindowsFile.h"
#include <windows.h>

String WindowsFileSystemFunctions::applicationDirectory(String &appName)
{
    String path;
    path.resize(MAX_PATH);
    dword pathActualSize = (dword)path.length();
    pathActualSize = GetModuleFileNameA(nullptr, path.data(), pathActualSize);
    
    path.resize(pathActualSize);
    WindowsFile file{ path };
    appName = file.getFileName();
    return file.getHostDirectory();
}

bool WindowsFileSystemFunctions::moveFile(GenericFile* moveFrom, GenericFile* moveTo)
{
    return MoveFileA(moveFrom->getFullPath().getChar(), moveTo->getFullPath().getChar());
}

bool WindowsFileSystemFunctions::copyFile(GenericFile* copyFrom, GenericFile* copyTo)
{
    return CopyFileA(copyFrom->getFullPath().getChar(), copyTo->getFullPath().getChar(), true);
}

bool WindowsFileSystemFunctions::replaceFile(GenericFile* replaceWith, GenericFile* replacing, GenericFile* backupFile)
{
    return ReplaceFileA(replacing->getFullPath().getChar(), replaceWith->getFullPath().getChar(),
        backupFile ? backupFile->getFullPath().getChar() : nullptr,
        REPLACEFILE_IGNORE_ACL_ERRORS | REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
}
