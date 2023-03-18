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
#include "Serialization/CommonTypesSerialization.h"

void ArchiveSizeCounterStream::read(void *, SizeT) { fatalAssertf(false, "Reading is not allowed in Size counter stream"); }
uint8 ArchiveSizeCounterStream::readForwardAt(SizeT) const
{
    fatalAssertf(false, "Reading is not allowed in Size counter stream");
    return 0;
}
uint8 ArchiveSizeCounterStream::readBackwardAt(SizeT) const
{
    fatalAssertf(false, "Reading is not allowed in Size counter stream");
    return 0;
}

STRINGID_CONSTEXPR static const StringID ARCHIVE_MARKER = STRID("CBEArchive");
void ArchiveBase::serializeArchiveMeta()
{
    // Mark as valid archive
    StringID archiveMarker = ARCHIVE_MARKER;
    (*this) << archiveMarker;
    fatalAssertf(archiveMarker == ARCHIVE_MARKER, "Invalid archive(No archive marker found)!");

    // Handle archive versions
    uint32 vers = ARCHIVE_VERSION;
    (*this) << vers;
    fatalAssertf(vers >= CUTOFF_VERSION, "Unsupported archive!");

    // Serialize custom versions
    (*this) << customVersions;
}
