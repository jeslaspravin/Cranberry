/*!
 * \file PackageData.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "String/StringID.h"

namespace CBE
{
class Object;
}

inline constexpr static const uint32 PACKAGE_SERIALIZER_VERSION = 0;
inline constexpr static const uint32 PACKAGE_SERIALIZER_CUTOFF_VERSION = 0;
inline STRINGID_CONSTEXPR static const StringID PACKAGE_CUSTOM_VERSION_ID = STRID("PackageSerializer");

// This will be flag set on object index when an object index is serialized to archive
inline constexpr static const SizeT DEPENDENT_OBJECT_FLAG = 0x80'00'00'00'00'00'00'00ull;
inline constexpr static const SizeT NULL_OBJECT_FLAG = ~0ull;

struct PackageDependencyData
{
    // Necessary as string to support package and object path processing
    String objectFullPath;
    StringID className;

    // Loaded/saving object
    CBE::Object *object = nullptr;
};

template <ArchiveType ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, PackageDependencyData &value)
{
    archive << value.objectFullPath;
    archive << value.className;
    return archive;
}

struct PackageContainedData
{
    // Without package path as package path will be derived from package itself
    String objectPath;
    StringID className;
    // Will also be pushed to archive's custom version, Should we need this?
    uint32 classVersion;

    SizeT streamStart;
    SizeT streamSize;

    // Loaded/saving object
    CBE::Object *object = nullptr;
};

template <ArchiveType ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, PackageContainedData &value)
{
    archive << value.objectPath;
    archive << value.className;
    archive << value.classVersion;

    archive << value.streamStart;
    archive << value.streamSize;

    return archive;
}