/*!
 * \file GenericFile.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/LFS/File/GenericFile.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/PlatformLFS.h"

void GenericFile::setPath(const String &fPath)
{
    String fPathTmp = PathFunctions::asGenericPath(fPath);

    // reverse find from last
    size_t hostDirectoryAt = fPathTmp.rfind(TCHAR('/'), fPathTmp.length());
    if (hostDirectoryAt != String::npos)
    {
        directoryPath = { fPathTmp.substr(0, hostDirectoryAt) };
        // Skip the separator char so +1
        fileName = { fPathTmp.substr(hostDirectoryAt + 1) };

        if (fileName.rfind(TCHAR('.')) == String::npos)
        {
            fileName = TCHAR("");
        }
        fullPath = fPathTmp;
    }
    else // No directory separator found so it must be just file name
    {
        LOG_ERROR("File", "File path \"{}\" is invalid", fPathTmp);
        debugAssert(!"File path is invalid");
    }
}

GenericFile::GenericFile()
    : fileHandle(nullptr)
    , directoryPath()
    , fileName()
    , fullPath()
    , fileFlags(0)
    , sharingMode(0)
    , attributes(0)
    , advancedFlags(0)
{}

GenericFile::GenericFile(const String &path)
    : fileHandle(nullptr)
    , fileFlags(0)
    , sharingMode(0)
    , attributes(0)
    , advancedFlags(0)
{
    if (path.length() > 0)
    {
        setPath(path);
    }
}

bool GenericFile::openOrCreate()
{
    if (fileHandle)
    {
        return true;
    }
    fileHandle = openOrCreateImpl();
    return fileHandle != nullptr;
}

bool GenericFile::openFile()
{
    if (fileHandle)
    {
        return true;
    }
    fileHandle = openImpl();
    return fileHandle != nullptr;
}

bool GenericFile::closeFile()
{
    if (fileHandle && closeImpl())
    {
        fileHandle = nullptr;
        return true;
    }
    return false;
}

bool GenericFile::exists() const
{
    if (getFileName().compare(TCHAR(".")) == 0 || getFileName().compare(TCHAR("..")) == 0) // not valid files or folders
    {
        return false;
    }

    if (isDirectory())
    {
        return FileSystemFunctions::dirExists(getFullPath().getChar());
    }

    return FileSystemFunctions::fileExists(getFullPath().getChar());
}

bool GenericFile::isDirectory() const { return fileName.empty(); }

bool GenericFile::isFile() const { return !isDirectory(); }

const String &GenericFile::getFileName() const { return fileName; }

const String &GenericFile::getHostDirectory() const { return directoryPath; }

String GenericFile::getDirectoryName() const
{
    String dirName;
    if (isDirectory())
    {
        size_t directoryAt = fullPath.rfind('/', fullPath.length());
        if (directoryAt != String::npos)
        {
            // Skip the separator char so +1
            dirName = { fullPath.substr(directoryAt + 1) };
        }
    }
    else
    {
        size_t directoryAt = directoryPath.rfind('/', directoryPath.length());
        if (directoryAt != String::npos)
        {
            // Skip the separator char so +1
            dirName = { directoryPath.substr(directoryAt + 1) };
        }
    }
    return dirName;
}

const String &GenericFile::getFullPath() const { return fullPath; }

void GenericFile::setFileFlags(uint8 flags)
{
    uint8 accessFlags = flags & FileFlags::ACCESS_FLAGS;
    uint8 actionFlags = flags & FileFlags::OPEN_ACTION_FLAGS;

    if (!ONE_BIT_SET(actionFlags))
    {
        actionFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;
    }

    fileFlags = accessFlags | actionFlags;
}

void GenericFile::setCreationAction(uint8 creationAction)
{
    uint8 actionFlags = creationAction & FileFlags::OPEN_ACTION_FLAGS;

    if (!ONE_BIT_SET(actionFlags))
    {
        actionFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;
    }
    else
    {
        removeFileFlags(FileFlags::OPEN_ACTION_FLAGS);
    }

    fileFlags |= actionFlags;
}

void GenericFile::addAdvancedFlags(uint64 flags) { advancedFlags |= flags; }

void GenericFile::removeAdvancedFlags(uint64 flags) { advancedFlags &= ~flags; }

void GenericFile::addSharingFlags(uint8 sharingFlags) { sharingMode |= sharingFlags; }

void GenericFile::removeSharingFlags(uint8 sharingFlags) { sharingMode &= ~sharingFlags; }

void GenericFile::addFileFlags(uint8 flags)
{
    uint8 accessFlags = flags & FileFlags::ACCESS_FLAGS;
    uint8 actionFlags = flags & FileFlags::OPEN_ACTION_FLAGS;

    if (ONE_BIT_SET(actionFlags))
    {
        removeFileFlags(FileFlags::OPEN_ACTION_FLAGS);
    }
    else
    {
        actionFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;
    }

    fileFlags |= accessFlags | actionFlags;
}

void GenericFile::removeFileFlags(uint8 flags) { fileFlags &= ~flags; }

void GenericFile::addAttributes(uint32 attribs) { attributes |= attribs; }

void GenericFile::removeAttributes(uint32 attribs) { attributes &= ~attribs; }
