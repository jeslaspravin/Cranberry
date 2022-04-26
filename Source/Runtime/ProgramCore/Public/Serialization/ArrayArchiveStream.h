/*!
 * \file ArrayArchiveStream.h
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

class PROGRAMCORE_EXPORT ArrayArchiveStream : public ArchiveStream
{
private:
    std::vector<uint8> buffer;
    SizeT cursor;

public:
    ArrayArchiveStream() = default;

    /* ArrayArchiveStream overrides */
    void read(void *toPtr, SizeT len) override;
    void write(const void *ptr, SizeT len) override;
    void moveForward(SizeT count) override;
    void moveBackward(SizeT count) override;
    bool allocate(SizeT count) override;
    uint8 readForwardAt(SizeT idx) const override;
    uint8 readBackwardAt(SizeT idx) const override;
    uint64 cursorPos() const override;
    bool isAvailable() const override;
    bool hasMoreData(SizeT requiredSize) const override;
    /* Overrides ends */
};