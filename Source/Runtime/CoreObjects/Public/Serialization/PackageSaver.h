/*!
 * \file PackageSaver.h
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

class PackageSaver : public ObjectArchive
{
private:
    cbe::Package *package;

    std::unordered_map<StringID, SizeT> objToContObjsIdx;
    std::vector<PackageContainedData> containedObjects;

    std::unordered_map<StringID, SizeT> objToDepObjsIdx;
    std::vector<PackageDependencyData> dependentObjects;

    BinaryArchive packageArchive;

public:
    PackageSaver(cbe::Package *savingPackage);
    EPackageLoadSaveResult savePackage();

    /* ObjectArchive overrides */
    ObjectArchive &serialize(cbe::Object *&obj) override;
    /* Overrides ends */

private:
    void setupContainedObjs();
    // Just helper to bring serializing object bytes to single place
    void serializeObject(cbe::Object *obj);
};
