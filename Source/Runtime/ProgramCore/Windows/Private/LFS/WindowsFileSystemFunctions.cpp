/*!
 * \file WindowsFileSystemFunctions.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "String/String.h"
#include "LFS/WindowsFileSystemFunctions.h"
#include "LFS/File/WindowsFile.h"
#include "WindowsCommonHeaders.h"
#include "Types/Platform/LFS/PathFunctions.h"

#include <queue>

std::vector<String> WindowsFileSystemFunctions::listFiles(const String &directory, bool bRecursive, const String &wildcard)
{
    std::vector<String> fileList;
    {
        WindowsFile rootDirectory(directory);
        if (!rootDirectory.isDirectory() || !rootDirectory.exists())
        {
            return fileList;
        }
    }

    std::vector<String> directories;
    directories.emplace_back(directory);
    // If we recurse find and append all subdirectories
    if (bRecursive)
    {
        auto subdirs = listAllDirectories(directory, bRecursive);
        directories.insert(directories.end(), subdirs.cbegin(), subdirs.cend());
    }

    for (const String &currentDir : directories)
    {
        WIN32_FIND_DATA data;
        HANDLE fHandle = ::FindFirstFile(PathFunctions::combinePath(currentDir, wildcard).c_str(), &data);

        if (fHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                String path = PathFunctions::combinePath(currentDir, data.cFileName);
                if (BIT_NOT_SET(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
                {
                    fileList.push_back(path);
                }
            }
            while (::FindNextFile(fHandle, &data));
            ::FindClose(fHandle);
        }
    }
    return fileList;
}

std::vector<String> WindowsFileSystemFunctions::listAllFiles(const String &directory, bool bRecursive)
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

        WIN32_FIND_DATA data;
        HANDLE fHandle = ::FindFirstFile(PathFunctions::combinePath(currentDir, TCHAR("*")).c_str(), &data);

        if (fHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                String path = PathFunctions::combinePath(currentDir, data.cFileName);
                String fileName = data.cFileName;
                // To replace . and .. that is part of file system redirectors
                fileName.replaceAll(TCHAR("."), TCHAR(""));
                if (BIT_NOT_SET(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
                {
                    fileList.push_back(path);
                }
                else if (bRecursive && !fileName.empty())
                {
                    directories.push(path);
                }
            }
            while (::FindNextFile(fHandle, &data));
            ::FindClose(fHandle);
        }
    }
    return fileList;
}

std::vector<String> WindowsFileSystemFunctions::listAllDirectories(const String &directory, bool bRecursive)
{
    std::vector<String> folderList;
    {
        WindowsFile rootDirectory(directory);
        if (!rootDirectory.isDirectory() || !rootDirectory.exists())
        {
            return folderList;
        }
    }

    std::queue<String> directories;
    directories.push(directory);

    while (!directories.empty())
    {
        String currentDir = directories.front();
        directories.pop();

        WIN32_FIND_DATA data;
        HANDLE fHandle = ::FindFirstFile(PathFunctions::combinePath(currentDir, TCHAR("*")).c_str(), &data);

        if (fHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                String path = PathFunctions::combinePath(currentDir, data.cFileName);
                String fileName = data.cFileName;
                // To replace . and .. that is part of file system redirectors
                fileName.replaceAll(TCHAR("."), TCHAR(""));
                if (BIT_SET(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) && !fileName.empty())
                {
                    folderList.emplace_back(path);
                    if (bRecursive)
                    {
                        directories.push(path);
                    }
                }
            }
            while (::FindNextFile(fHandle, &data));
            ::FindClose(fHandle);
        }
    }
    return folderList;
}

String WindowsFileSystemFunctions::applicationPath()
{
    String path;
    path.resize(MAX_PATH);
    dword pathActualSize = (dword)path.length();
    pathActualSize = ::GetModuleFileName(nullptr, path.data(), pathActualSize);
    return path;
}

bool WindowsFileSystemFunctions::moveFile(GenericFile *moveFrom, GenericFile *moveTo)
{
    return ::MoveFile(moveFrom->getFullPath().getChar(), moveTo->getFullPath().getChar());
}

bool WindowsFileSystemFunctions::copyFile(GenericFile *copyFrom, GenericFile *copyTo)
{
    return ::CopyFile(copyFrom->getFullPath().getChar(), copyTo->getFullPath().getChar(), true);
}

bool WindowsFileSystemFunctions::replaceFile(GenericFile *replaceWith, GenericFile *replacing, GenericFile *backupFile)
{
    return ::ReplaceFile(
        replacing->getFullPath().getChar(), replaceWith->getFullPath().getChar(), backupFile ? backupFile->getFullPath().getChar() : nullptr,
        REPLACEFILE_IGNORE_ACL_ERRORS | REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr
    );
}

bool WindowsFileSystemFunctions::exists(const TChar *fullPath)
{
    dword fType = ::GetFileAttributes(fullPath);
    return fType != INVALID_FILE_ATTRIBUTES;
}

bool WindowsFileSystemFunctions::fileExists(const TChar *fullPath)
{
    dword fType = ::GetFileAttributes(fullPath);

    if (fType == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    return BIT_NOT_SET(fType, FILE_ATTRIBUTE_DIRECTORY);
}

bool WindowsFileSystemFunctions::dirExists(const TChar *fullPath)
{
    dword fType = ::GetFileAttributes(fullPath);

    if (fType == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    return BIT_SET(fType, FILE_ATTRIBUTE_DIRECTORY);
}
