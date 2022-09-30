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

class ArrayArchiveStream;

namespace cbe
{
class Package;
}

class PackageSaver final : public ObjectArchive
{
private:
    cbe::Package *package;

    std::unordered_map<StringID, SizeT> objToContObjsIdx;
    std::vector<PackageContainedData> containedObjects;

    std::unordered_map<StringID, SizeT> objToDepObjsIdx;
    std::vector<PackageDependencyData> dependentObjects;

    BinaryArchive packageArchive;
    // Only should be set if not going to serialize to file by default
    ArrayArchiveStream *outStream = nullptr;

public:
    PackageSaver(cbe::Package *savingPackage);
    EPackageLoadSaveResult savePackage();

    /* ObjectArchive overrides */
    void relinkSerializedPtr(void **objPtrPtr) const final
    {
        // Nothing to link
    }
    void relinkSerializedPtr(const void **objPtrPtr) const final
    {
        // Nothing to link
    }
    ObjectArchive &serialize(cbe::Object *&obj) final;
    /* Overrides ends */

    void setOutStreamer(ArrayArchiveStream *stream) { outStream = stream; }

private:
    void setupContainedObjs();
    // Just helper to bring serializing object bytes to single place
    void serializeObject(cbe::WeakObjPtr<cbe::Object> obj);
};
