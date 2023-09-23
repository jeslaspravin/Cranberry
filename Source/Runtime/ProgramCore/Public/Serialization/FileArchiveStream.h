/*!
 * \file FileArchiveStream.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/ArchiveBase.h"

class GenericFile;

class PROGRAMCORE_EXPORT FileArchiveStream : public ArchiveStream
{
private:
    GenericFile *file;
    uint64 fileCursor;
    bool bIsReadOnly;
    bool bIsOpened;

public:
    FileArchiveStream(const String &filePath, bool bReading);
    ~FileArchiveStream();

    /* FileArchiveStream overrides */
    void read(void *toPtr, SizeT byteLen) override;
    void write(const void *ptr, SizeT byteLen) override;
    void moveForward(SizeT byteCount) override;
    void moveBackward(SizeT byteCount) override;
    bool allocate(SizeT byteCount) override;
    uint8 readForwardAt(SizeT idx) const override;
    uint8 readBackwardAt(SizeT idx) const override;
    uint64 cursorPos() const override;
    bool isAvailable() const override;
    bool hasMoreData(SizeT requiredByteCount) const override;
    /* Overrides ends */
};