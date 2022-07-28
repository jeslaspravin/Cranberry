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

namespace cbe
{
class Package;
}

class PackageLoader : public ObjectArchive
{
private:
    cbe::Package *package;
    String packageFilePath;

    std::vector<PackageContainedData> containedObjects;
    std::vector<PackageDependencyData> dependentObjects;

    SizeT streamStartAt;

    BinaryArchive packageArchive;

private:
    /**
     * Creates or obtains objects contained in this package and sets it in corresponding PackageContainedData
     * For transient objects this will set object to found object. If no transient object exists it will be nullptr
     * Returns collectedFlags from all outers
     */
    EObjectFlags createContainedObject(PackageContainedData &containedData);

public:
    PackageLoader(cbe::Package *loadingPackage, const String &filePath);
    MAKE_TYPE_NONCOPY_NONMOVE(PackageLoader)

    /* ObjectArchive overrides */
    ObjectArchive &serialize(cbe::Object *&obj) override;
    /* Overrides ends */

    /**
     * Prepares loader for the package.
     * Loads package header tables
     */
    void prepareLoader();
    EPackageLoadSaveResult load();

    FORCE_INLINE cbe::Package *getPackage() const { return package; }
    FORCE_INLINE const std::vector<PackageContainedData> &getContainedObjects() const { return containedObjects; }
};