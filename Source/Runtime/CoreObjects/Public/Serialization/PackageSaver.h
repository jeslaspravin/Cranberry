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

namespace CBE
{
class Package;
}

class PackageSaver : public ObjectArchive
{
private:
    CBE::Package *package;

    std::unordered_map<StringID, SizeT> objToContObjsIdx;
    std::vector<PackageContainedData> containedObjects;

    std::unordered_map<StringID, SizeT> objToDepObjsIdx;
    std::vector<PackageDependencyData> dependentObjects;

    BinaryArchive packageArchive;

private:
    void setupContainedObjs();

public:
    PackageSaver(CBE::Package *savingPackage);
    EPackageLoadSaveResult savePackage();

    /* ObjectArchive overrides */
    ObjectArchive &serialize(CBE::Object *&obj) override;
    /* Overrides ends */
};
