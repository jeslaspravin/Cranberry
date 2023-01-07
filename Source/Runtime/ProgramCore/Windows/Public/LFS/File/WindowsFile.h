/*!
 * \file WindowsFile.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Platform/LFS/File/GenericFile.h"

class PROGRAMCORE_EXPORT WindowsFile final : public GenericFile
{

public:
    WindowsFile(const String &path)
        : GenericFile(path)
    {}
    WindowsFile()
        : GenericFile(TCHAR(""))
    {}
    WindowsFile(WindowsFile &&otherFile);
    WindowsFile(const WindowsFile &otherFile);
    ~WindowsFile();
    void operator=(const WindowsFile &otherFile);
    void operator=(WindowsFile &&otherFile);

    void flush() const override;

    TickRep lastWriteTimeStamp() const override;
    bool setLastWriteTimeStamp(TickRep timeTick) const override;
    TickRep createTimeStamp() const override;
    uint64 fileSize() const override;
    uint64 filePointer() const override;
    void seekEnd() const override;
    void seekBegin() const override;
    void seek(int64 pointer) const override;
    void offsetCursor(int64 offset) const override;

    bool setFileSize(int64 newSize) const override;
    void read(std::vector<uint8> &readTo, uint32 bytesToRead = (~0u)) const override;
    void read(uint8 *readTo, uint32 bytesToRead) const override;
    void write(const ArrayView<const uint8> &writeBytes) const override;

    bool deleteFile() override;
    bool renameFile(String newName) override;

    bool createDirectory() const override;

protected:
    virtual PlatformHandle openOrCreateImpl() override;
    virtual PlatformHandle openImpl() const override;
    virtual bool closeImpl() const override;

    bool dirDelete() const override;
    bool dirClearAndDelete() const override;
};

namespace LFS
{
typedef WindowsFile PlatformFile;
}