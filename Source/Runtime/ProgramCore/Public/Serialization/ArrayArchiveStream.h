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
    SizeT cursor = 0;

public:
    ArrayArchiveStream() = default;

    /* ArrayArchiveStream overrides */
    void read(void *toPtr, SizeT byteLen) override;
    void write(const void *ptr, SizeT byteLen) override;
    void moveForward(SizeT byteCount) override;
    void moveBackward(SizeT byteCount) override;
    bool allocate(SizeT byteCount) override;
    uint8 readForwardAt(SizeT idx) const override;
    uint8 readBackwardAt(SizeT idx) const override;
    uint64 cursorPos() const override;
    bool isAvailable() const override { return true; }
    bool hasMoreData(SizeT requiredByteCount) const override;
    /* Overrides ends */

    void setBuffer(const std::vector<uint8> &inBuffer) { buffer = inBuffer; }
    const std::vector<uint8> &getBuffer() const { return buffer; }
};