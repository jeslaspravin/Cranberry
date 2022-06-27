/*!
 * \file ArchiveBase.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/ArchiveBase.h"
#include "Types/Platform/PlatformAssertionErrors.h"

void ArchiveSizeCounterStream::read(void *toPtr, SizeT byteLen) { fatalAssertf(false, "Reading is not allowed in Size counter stream"); }
uint8 ArchiveSizeCounterStream::readForwardAt(SizeT idx) const
{
    fatalAssertf(false, "Reading is not allowed in Size counter stream");
    return 0;
}
uint8 ArchiveSizeCounterStream::readBackwardAt(SizeT idx) const
{
    fatalAssertf(false, "Reading is not allowed in Size counter stream");
    return 0;
}

void ArchiveBase::serializeArchiveMeta()
{
    uint64 vers = ARCHIVE_VERSION;
    (*this) << vers;
    fatalAssertf(vers >= CUTOFF_VERSION, "Unsupported archive!");

    (*this) << customVersions;
}
