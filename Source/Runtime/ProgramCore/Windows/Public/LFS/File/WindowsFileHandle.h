/*!
 * \file WindowsFileHandle.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"
#include "Types/Platform/LFS/File/GenericFileHandle.h"

class PROGRAMCORE_EXPORT WindowsFileHandle final : public GenericFileHandle
{
private:
    void *fileHandle = nullptr;

protected:
    void *getFileHandle() override;

public:
    WindowsFileHandle(uint8 flags, uint8 sharing = (uint8)EFileSharing::ReadOnly, uint32 extraFlags = 0,
        uint64 advancedFlags = 0);

    ~WindowsFileHandle();

    bool openFile(const String &filePath);
    bool closeFile();
};