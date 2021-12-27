#include "LFS/WindowsFileSystemFunctions.h"
#include "LFS/File/WindowsFile.h"
#include "WindowsCommonHeaders.h"

#include <queue>

FORCE_INLINE std::vector<String> WindowsFileSystemFunctions::listFiles(const String& directory, bool bRecursive, const String& wildcard)
{
    std::vector<String> fileList;
    {
        WindowsFile rootDirectory(directory);
        if (!rootDirectory.isDirectory() || !rootDirectory.exists())
        {
            return fileList;
        }
    }

    std::queue<String> directories;
    directories.push(directory);

    while (!directories.empty())
    {
        String currentDir = directories.front();
        directories.pop();

        WIN32_FIND_DATAA data;
        HANDLE fHandle = FindFirstFileA(combinePath(currentDir, wildcard).c_str(), &data);

        if (fHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                String path = combinePath(currentDir, data.cFileName);
                String fileName = data.cFileName;
                // To replace . and .. that is part of file system redirectors
                fileName.replaceAll(".", "");
                if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    fileList.push_back(path);
                }
                else if (bRecursive && !fileName.empty())
                {
                    directories.push(path);
                }

            } while (FindNextFileA(fHandle, &data));
            FindClose(fHandle);
        }
    }
    return fileList;
}

std::vector<String> WindowsFileSystemFunctions::listAllFiles(const String& directory, bool bRecursive)
{
    return listFiles(directory, bRecursive, "*");
}

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
