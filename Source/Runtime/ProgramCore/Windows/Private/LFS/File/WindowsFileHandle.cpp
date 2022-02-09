/*!
 * \file WindowsFileHandle.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "LFS/File/WindowsFileHandle.h"
#include "WindowsCommonHeaders.h"
#include "Types/CoreDefines.h"
#include "Logger/Logger.h"

void* WindowsFileHandle::getFileHandle()
{
    return fileHandle;
}

WindowsFileHandle::WindowsFileHandle(uint8 flags, uint8 sharing /*= (uint8)EFileSharing::ReadOnly*/, 
    uint32 extraFlags /*= 0*/, uint64 advancedFlags /*= 0*/):GenericFileHandle(flags,sharing,extraFlags,advancedFlags)
{

}

#define FLAG_CHECK_ASSIGN(OutFlags,InFlags,CheckAgainst,Result) OutFlags |= (InFlags & CheckAgainst) > 0 ? Result : 0
bool WindowsFileHandle::openFile(const String& filePath)
{
    uint32 desiredAccess = 0;
    uint32 shareMode = 0;
    uint32 creationAction = OPEN_ALWAYS;
    uint32 attributsAndFlags = 0;

    {
        if ((FileFlags::ACCESS_FLAGS & fileFlags) == FileFlags::ACCESS_FLAGS)
        {
            desiredAccess |= GENERIC_ALL;
        }
        else
        {
            FLAG_CHECK_ASSIGN(desiredAccess, fileFlags, EFileFlags::Read, GENERIC_READ);
            FLAG_CHECK_ASSIGN(desiredAccess, fileFlags, EFileFlags::Write, GENERIC_WRITE);
            FLAG_CHECK_ASSIGN(desiredAccess, fileFlags, EFileFlags::Execute, GENERIC_EXECUTE);
        }
    }

    {
        FLAG_CHECK_ASSIGN(shareMode, fileSharing, EFileSharing::DeleteOnly, FILE_SHARE_DELETE);
        FLAG_CHECK_ASSIGN(shareMode, fileSharing, EFileSharing::ReadOnly, FILE_SHARE_READ);
        FLAG_CHECK_ASSIGN(shareMode, fileSharing, EFileSharing::WriteOnly, FILE_SHARE_WRITE);
    }

    {
        uint8 actionsFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;

        if (ONE_BIT_SET(actionsFlags))// If only one bit is set
        {
            uint8 currentFlag = EFileFlags::CreateNew;
            uint32 count = 1;
            while (FileFlags::OPEN_ACTION_FLAGS & currentFlag)
            {
                if (actionsFlags & currentFlag) {
                    creationAction = count;// Since the order is same as int value in windows API
                    break;
                }
                currentFlag <<= 1;
                count++;
            }
        }
    }

    {
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Normal, FILE_ATTRIBUTE_NORMAL);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Archive, FILE_ATTRIBUTE_ARCHIVE);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Encrypted, FILE_ATTRIBUTE_ENCRYPTED);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Hidden, FILE_ATTRIBUTE_HIDDEN);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::ReadOnly, FILE_ATTRIBUTE_READONLY);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::System, FILE_ATTRIBUTE_SYSTEM);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Temporary, FILE_ATTRIBUTE_TEMPORARY);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Backup, FILE_FLAG_BACKUP_SEMANTICS);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::TemporaryDelete, FILE_FLAG_DELETE_ON_CLOSE);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::NoBuffering, FILE_FLAG_NO_BUFFERING);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::OpenRemoteOnly, FILE_FLAG_OPEN_NO_RECALL);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::AsyncOverlapped, FILE_FLAG_OVERLAPPED);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Posix, FILE_FLAG_POSIX_SEMANTICS);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::RandomAccess, FILE_FLAG_RANDOM_ACCESS);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::SequentialAccess, FILE_FLAG_SEQUENTIAL_SCAN);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::WriteDirectDisk, FILE_FLAG_WRITE_THROUGH);

        attributsAndFlags |= rawFileFlags;
    }

    fileHandle = CreateFile(filePath.getChar(), desiredAccess, shareMode, nullptr, creationAction, attributsAndFlags, nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("WindowsFileHandle", "%s() : File handle creation/opening failed for %s", __func__, filePath.getChar());
        fileHandle = nullptr;
        return false;
    }
    return true;
}
#undef FLAG_CHECK_ASSIGN

bool WindowsFileHandle::closeFile()
{
    bool success = false;
    if (!fileHandle)
    {
        return success;
    } 
    
    success = CloseHandle(fileHandle) != 0;

    if (success) {
        fileHandle = nullptr;
    }

    return success;
}

WindowsFileHandle::~WindowsFileHandle()
{
    closeFile();
}
