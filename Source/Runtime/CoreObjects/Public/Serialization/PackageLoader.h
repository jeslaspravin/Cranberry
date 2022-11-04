/*!
 * \file PackageLoader.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/PackageData.h"
#include "Serialization/ObjectArchive.h"
#include "Serialization/BinaryArchive.h"

class ArrayArchiveStream;

namespace cbe
{
class Package;
class Object;
} // namespace cbe

class PackageLoader final : public ObjectArchive
{
private:
    static_assert(sizeof(UPtrInt) == 8, "Change below sentinel value for delay link pointer!");
    constexpr static const UPtrInt SENTINEL_LINK_PTR = 0xCDCDCDCDCDCDCDCD;

    cbe::Package *package;
    String packageFilePath;

    std::vector<PackageContainedData> containedObjects;
    std::vector<PackageDependencyData> dependentObjects;
    UPtrInt delayLinkPtrMask = 0;
    SizeT streamStartAt;

    BinaryArchive packageArchive;
    // Only should be set if not going to serialize from file by default
    ArrayArchiveStream *inStream = nullptr;

    bool bDelayLinkRequired = false;

private:
    /**
     * Creates or obtains objects contained in this package and sets it in corresponding PackageContainedData
     * For transient objects this will set object to found object. If no transient object exists it will be nullptr
     * Returns collectedFlags from all outers
     */
    EObjectFlags createContainedObject(PackageContainedData &containedData);
    template <typename T>
    FORCE_INLINE void relinkLoadedPtr(T **objPtrPtr) const;
    FORCE_INLINE void linkContainedObjects() const;

public:
    PackageLoader(cbe::Package *loadingPackage, const String &filePath);
    MAKE_TYPE_NONCOPY_NONMOVE(PackageLoader)

    /* ObjectArchive overrides */
    void relinkSerializedPtr(void **objPtrPtr) const final;
    void relinkSerializedPtr(const void **objPtrPtr) const final;
    ObjectArchive &serialize(cbe::Object *&obj) final;
    /* Overrides ends */

    /**
     * Prepares loader for the package.
     * Loads package header tables
     */
    void prepareLoader();
    EPackageLoadSaveResult load();
    void unload();

    void setInStreamer(ArrayArchiveStream *stream) { inStream = stream; }

    FORCE_INLINE cbe::Package *getPackage() const { return package; }
    FORCE_INLINE const std::vector<PackageContainedData> &getContainedObjects() const { return containedObjects; }
};