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
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Serialization/ArrayArchiveStream.h"
#include "ObjectPathHelpers.h"
#include "CoreObjectsDB.h"
#include "ICoreObjectsModule.h"
#include "CBEPackage.h"
#include "CoreObjectDelegates.h"

void PackageSaver::setupContainedObjs()
{
    String packageFullPath = package->getFullPath();
    const CoreObjectsDB &objsDb = ICoreObjectsModule::get()->getObjectsDB();

    // We peel the onion as parent must be create before child,
    // The getChildren from FlatTree already returns in ordered manner so we should be good without peeling manually here
    std::vector<cbe::Object *> children;
    objsDb.getSubobjects(children, { .objectPath = packageFullPath.getChar(), .objectId = package->getStringID() });
    containedObjects.clear();
    containedObjects.reserve(children.size());
    for (cbe::Object *child : children)
    {
        // Package is final class so we just have compare, no need to go through isChild hierarchy
        fatalAssertf(child->getType() != cbe::Package::staticType(), "Package must not contain package object");
        if (ANY_BIT_SET(child->getFlags(), cbe::EObjectFlagBits::ObjFlag_MarkedForDelete | cbe::EObjectFlagBits::ObjFlag_Deleted))
        {
            continue;
        }

        objToContObjsIdx[child->getFullPath().getChar()] = containedObjects.size();
        PackageContainedData &containedObjData = containedObjects.emplace_back();
        containedObjData.object = child;
        containedObjData.objectPath = ObjectPathHelper::getObjectPath(child, package);
        containedObjData.objectFlags = child->getFlags();
        // No need for dirty flags to be serialized out
        CLEAR_BITS(containedObjData.objectFlags, cbe::EObjectFlagBits::ObjFlag_PackageDirty);
        containedObjData.clazz = child->getType();
    }
}

void PackageSaver::serializeObject(cbe::WeakObjPtr<cbe::Object> obj)
{
    debugAssert(obj.isValid());

    /**
     * If transient we store the object as part of package but never serialize it.
     * This is to allow us to do pointer fix ups if transient object is available while loading
     * Collecting all parent object tree so that when loading we do not depend on transient object being available at object creation
     */
    if (NO_BITS_SET(obj->collectAllFlags(), cbe::EObjectFlagBits::ObjFlag_Transient))
    {
        obj->serialize(*this);
    }
}

PackageSaver::PackageSaver(cbe::Package *savingPackage)
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
        serializeObject(containedObjData.object);
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

    // Step 5 : Setup Array stream to write
    ArrayArchiveStream archiveStream;
    ArrayArchiveStream *archiveStreamPtr = outStream ? outStream : &archiveStream;
    archiveStreamPtr->allocate(finalPackageSize);
    packageArchive.setStream(archiveStreamPtr);

    // Step 6 : Write into archive
    (*static_cast<ObjectArchive *>(this)) << *const_cast<StringID *>(&PACKAGE_ARCHIVE_MARKER);
    (*static_cast<ObjectArchive *>(this)) << containedObjects;
    (*static_cast<ObjectArchive *>(this)) << dependentObjects;
    for (PackageContainedData &containedObjData : containedObjects)
    {
        serializeObject(containedObjData.object);
    }
    packageArchive.setStream(nullptr);

    if (outStream == nullptr)
    {
        String packagePath = package->getPackageFilePath();
        if (!FileHelper::writeBytes(archiveStreamPtr->getBuffer(), packagePath))
        {
            LOG_ERROR("PackageSaver", "Failed to open file stream to save package %s at %s", package->getName(), packagePath);
            return EPackageLoadSaveResult::IOError;
        }
        CoreObjectDelegates::broadcastPackageSaved(package);
    }

    return EPackageLoadSaveResult::Success;
}

ObjectArchive &PackageSaver::serialize(cbe::Object *&obj)
{
    // Push null object index if object is null
    if (!obj)
    {
        (*static_cast<ObjectArchive *>(this)) << *const_cast<SizeT *>(&NULL_OBJECT_FLAG);
        return (*this);
    }

    auto containedObjItr = objToContObjsIdx.find(obj->getFullPath().getChar());
    if (containedObjItr != objToContObjsIdx.end())
    {
        (*static_cast<ObjectArchive *>(this)) << containedObjItr->second;
    }
    else
    {
        auto depObjItr = objToDepObjsIdx.find(obj->getFullPath().getChar());
        SizeT depObjIdx = 0;
        // If dependent is not there then we must create a new entry and serialize it
        if (depObjItr == objToDepObjsIdx.end())
        {
            depObjIdx = dependentObjects.size();
            objToDepObjsIdx[obj->getFullPath().getChar()] = depObjIdx;
            PackageDependencyData &objDepData = dependentObjects.emplace_back();
            objDepData.object = obj;
            objDepData.objectFullPath = obj->getFullPath();
            objDepData.clazz = obj->getType();
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
