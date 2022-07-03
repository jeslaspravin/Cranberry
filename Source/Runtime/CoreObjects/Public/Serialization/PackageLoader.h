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

namespace CBE
{
class Package;
}

class PackageLoader : public ObjectArchive
{
private:
    CBE::Package *package;
    String packageFilePath;

    std::vector<PackageContainedData> containedObjects;
    std::vector<PackageDependencyData> dependentObjects;

    SizeT streamStartAt;

    BinaryArchive packageArchive;

private:
    void createContainedObject(PackageContainedData &containedData);

public:
    PackageLoader(CBE::Package *loadingPackage, const String &filePath);
    MAKE_TYPE_NONCOPY_NONMOVE(PackageLoader)

    /* ObjectArchive overrides */
    ObjectArchive &serialize(CBE::Object *&obj) override;
    /* Overrides ends */

    /**
     * Prepares loader for the package.
     * Loads package header tables
     */
    void prepareLoader();
    EPackageLoadSaveResult load();

    FORCE_INLINE CBE::Package *getPackage() const { return package; }
    FORCE_INLINE const std::vector<PackageContainedData> &getContainedObjects() const { return containedObjects; }
};