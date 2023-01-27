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

#include "ObjectPtrs.h"

constexpr inline const uint32 PACKAGE_SERIALIZER_VERSION = 0;
constexpr inline const uint32 PACKAGE_SERIALIZER_CUTOFF_VERSION = 0;
STRINGID_CONSTEXPR inline const StringID PACKAGE_CUSTOM_VERSION_ID = STRID("PackageSerializer");
STRINGID_CONSTEXPR inline const StringID PACKAGE_ARCHIVE_MARKER = STRID("SerializedCBEPackage");

// This will be flag set on object index when an object index is serialized to archive
constexpr inline const SizeT DEPENDENT_OBJECT_FLAG = 0x80'00'00'00'00'00'00'00ull;
constexpr inline const SizeT NULL_OBJECT_FLAG = ~0ull;

struct PackageDependencyData
{
    // Necessary as string to support package and object path processing
    String objectFullPath;
    CBEClass clazz;

    // Loaded/saving object
    cbe::WeakObjPtr<cbe::Object> object;
};

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, PackageDependencyData &value)
{
    archive << value.objectFullPath;
    archive << value.clazz;
    return archive;
}

struct PackageContainedData
{
    // Without package path as package path will be derived from package itself
    String objectPath;
    // Will also be pushed to archive's custom version, Should we need this?
    uint32 classVersion;
    EObjectFlags objectFlags;
    CBEClass clazz;

    SizeT streamStart;
    SizeT streamSize;

    // Loaded/saving object
    cbe::WeakObjPtr<cbe::Object> object;
};

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, PackageContainedData &value)
{
    archive << value.objectPath;
    archive << value.classVersion;
    archive << value.objectFlags;
    archive << value.clazz;

    archive << value.streamStart;
    archive << value.streamSize;

    return archive;
}

enum class EPackageLoadSaveResult : uint32
{
    Failed = 0,
    IOError,
    WithWarnings,
    Success,
    ErrorStart = Failed,
    ErrorEnd = IOError
};

#define CBEPACKAGE_SAVELOAD_SUCCESS(OpResult) ((OpResult) == EPackageLoadSaveResult::Success)
#define CBEPACKAGE_SAVELOAD_ERROR(OpResult)                                                                                                    \
    (uint32(OpResult) >= uint32(EPackageLoadSaveResult::ErrorStart) && uint32(OpResult) <= uint32(EPackageLoadSaveResult::ErrorEnd))