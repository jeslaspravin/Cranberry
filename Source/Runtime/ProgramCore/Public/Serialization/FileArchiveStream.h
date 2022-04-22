/*!
 * \file FileArchiveStream.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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

public:
    FileArchiveStream(const String &filePath, bool bReading);
    ~FileArchiveStream();

    /* FileArchiveStream overrides */
    void read(void *toPtr, SizeT len) override;
    void write(const void *ptr, SizeT len) override;
    void moveForward(SizeT count) override;
    void moveBackward(SizeT count) override;
    bool allocate(SizeT count) override;
    uint8 readForwardAt(SizeT idx) const override;
    uint8 readBackwardAt(SizeT idx) const override;
    uint64 cursorPos() const override;
    bool isAvailable() const override;
    /* Overrides ends */
};