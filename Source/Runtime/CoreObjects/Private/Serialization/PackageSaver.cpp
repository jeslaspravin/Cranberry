/*!
 * \file PackageSaver.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/PackageSaver.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Serialization/FileArchiveStream.h"
#include "ObjectPathHelpers.h"
#include "CoreObjectsDB.h"
#include "ICoreObjectsModule.h"
#include "CBEPackage.h"
#include "CoreObjectDelegates.h"

void PackageSaver::setupContainedObjs()
{
    const CoreObjectsDB &objsDb = ICoreObjectsModule::get()->getObjectsDB();

    // We peel the onion as parent must be create before child,
    // The getChildren from FlatTree already returns in ordered manner so we should be good without peeling manually here
    std::vector<CBE::Object *> children;
    objsDb.getSubobjects(children, package->getStringID());
    for (CBE::Object *child : children)
    {
        // Package is final class so we just have compare, no need to go through isChild hierarchy
        fatalAssertf(child->getType() != CBE::Package::staticType(), "Package must not contain package object");

        objToContObjsIdx[child->getStringID()] = containedObjects.size();
        PackageContainedData &containedObjData = containedObjects.emplace_back();
        containedObjData.object = child;
        containedObjData.objectPath = ObjectPathHelper::getObjectPath(child, package);
        containedObjData.objectFlags = child->getFlags();
        containedObjData.className = child->getType()->name;
    }
}

PackageSaver::PackageSaver(CBE::Package *savingPackage)
    : package(savingPackage)
{
    debugAssert(package);

    setLoading(false);
    packageArchive.setLoading(false);
    // Maybe in future need to change this to swap based on platform to cook data to
    setSwapBytes(false);
    packageArchive.setSwapBytes(false);

    setInnerArchive(&packageArchive);
    setupContainedObjs();
}

EPackageLoadSaveResult PackageSaver::savePackage()
{
    ArchiveSizeCounterStream archiveCounter;
    packageArchive.setStream(&archiveCounter);
    // Step 1 : Dummy archive meta/header size for offsetting original stream start and size later
    SizeT dummyHeaderSize = archiveCounter.cursorPos();

    // Step 2 : Dummy serialize to find stream start and stream size for each object
    for (PackageContainedData &containedObjData : containedObjects)
    {
        containedObjData.streamStart = archiveCounter.cursorPos();
        containedObjData.object->serialize(*this);
        containedObjData.streamSize = archiveCounter.cursorPos() - containedObjData.streamStart;
        // We must have custom version setup if present, Custom version keys must be from class property name
        containedObjData.classVersion = ArchiveBase::getCustomVersion(uint32(containedObjData.object->getType()->name));
    }

    // Step 3 : Copy custom versions and other archive related property to actual packageArchive
    for (const std::pair<const uint32, uint32> &customVersion : ArchiveBase::getCustomVersions())
    {
        packageArchive.setCustomVersion(customVersion.first, customVersion.second);
    }
    packageArchive.setCustomVersion(uint32(PACKAGE_CUSTOM_VERSION_ID), PACKAGE_SERIALIZER_VERSION);

    // Step 4 : Now all custom versions and dependent object data are setup we determine new header size and set stream start and size offsets
    // for serialized objects
    packageArchive.setStream(nullptr);
    archiveCounter.moveBackward(archiveCounter.cursorPos());
    packageArchive.setStream(&archiveCounter);
    (*static_cast<ObjectArchive *>(this)) << *const_cast<StringID *>(&PACKAGE_ARCHIVE_MARKER);
    (*static_cast<ObjectArchive *>(this)) << containedObjects;
    (*static_cast<ObjectArchive *>(this)) << dependentObjects;
    SizeT actualHeaderSize = archiveCounter.cursorPos();
    for (PackageContainedData &containedObjData : containedObjects)
    {
        containedObjData.streamStart = (containedObjData.streamStart - dummyHeaderSize) + actualHeaderSize;
    }
    SizeT finalPackageSize = containedObjects.back().streamStart + containedObjects.back().streamSize;

    // Step 5 : Setup file stream to write
    String packagePath = package->getPackageFilePath();
    FileArchiveStream fileStream(packagePath, false);
    if (!fileStream.isAvailable())
    {
        LOG_ERROR("PackageSaver", "Failed to open file stream to save package %s at %s", package->getName(), packagePath);
        return EPackageLoadSaveResult::IOError;
    }
    fileStream.allocate(finalPackageSize);
    packageArchive.setStream(&fileStream);

    // Step 6 : Write into archive
    (*static_cast<ObjectArchive *>(this)) << *const_cast<StringID *>(&PACKAGE_ARCHIVE_MARKER);
    (*static_cast<ObjectArchive *>(this)) << containedObjects;
    (*static_cast<ObjectArchive *>(this)) << dependentObjects;
    for (PackageContainedData &containedObjData : containedObjects)
    {
        containedObjData.object->serialize(*this);
    }
    packageArchive.setStream(nullptr);

    CoreObjectDelegates::broadcastPackageSaved(package);

    return EPackageLoadSaveResult::Success;
}

ObjectArchive &PackageSaver::serialize(CBE::Object *&obj)
{
    // Push null object index
    if (!obj)
    {
        (*static_cast<ObjectArchive *>(this)) << *const_cast<SizeT *>(&NULL_OBJECT_FLAG);
        return (*this);
    }

    auto containedObjItr = objToContObjsIdx.find(obj->getStringID());
    if (containedObjItr != objToContObjsIdx.end())
    {
        (*static_cast<ObjectArchive *>(this)) << containedObjItr->second;
    }
    else
    {
        auto depObjItr = objToDepObjsIdx.find(obj->getStringID());
        SizeT depObjIdx = 0;
        // If dependent is not there then we must create a new entry and serialize it
        if (depObjItr == objToDepObjsIdx.end())
        {
            depObjIdx = dependentObjects.size();
            objToDepObjsIdx[obj->getStringID()] = depObjIdx;
            PackageDependencyData &objDepData = dependentObjects.emplace_back();
            objDepData.object = obj;
            objDepData.objectFullPath = obj->getFullPath();
            objDepData.className = obj->getType()->name;
        }
        else
        {
            depObjIdx = depObjItr->second;
        }
        SET_BITS(depObjIdx, DEPENDENT_OBJECT_FLAG);
        (*static_cast<ObjectArchive *>(this)) << depObjIdx;
    }
    return *this;
}
