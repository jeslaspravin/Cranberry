/*!
 * \file PackageSaver.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/PackageSaver.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Serialization/ArrayArchiveStream.h"
#include "ObjectPathHelpers.h"
#include "CoreObjectsDB.h"
#include "CoreObjectsModule.h"
#include "CBEPackage.h"
#include "CoreObjectDelegates.h"

void PackageSaver::setupContainedObjs()
{
    const CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();

    // We peel the onion as parent must be create before child,
    // The getChildren from FlatTree already returns in ordered manner so we should be good without peeling manually here
    std::vector<cbe::Object *> children;
    objsDb.getSubobjects(children, package->getDbIdx());
    containedObjects.clear();
    containedObjects.reserve(children.size());
    for (cbe::Object *child : children)
    {
        cbe::ObjectPrivateDataView childObjDatV = child->getObjectData();
        // Package is final class so we just have compare, no need to go through isChild hierarchy
        fatalAssertf(childObjDatV.clazz != cbe::Package::staticType(), "Package must not contain package object");
        if (ANY_BIT_SET(childObjDatV.flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete))
        {
            continue;
        }

        objToContObjsIdx[NameString(childObjDatV.path)] = containedObjects.size();
        PackageContainedData &containedObjData = containedObjects.emplace_back();
        containedObjData.object = child;
        containedObjData.objectPath = ObjectPathHelper::computeObjectPath(child, package);
        containedObjData.objectFlags = childObjDatV.flags;
        // No need for dirty flags to be serialized out
        CLEAR_BITS(containedObjData.objectFlags, cbe::EObjectFlagBits::ObjFlag_PackageDirty);
        containedObjData.clazz = childObjDatV.clazz;
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
    CBE_PROFILER_SCOPE("SavePackage");

    ArchiveSizeCounterStream archiveCounter;
    packageArchive.setStream(&archiveCounter);
    /**
     * STEP 1 :
     * Dummy archive meta/header size for offsetting original stream start and size later
     */
    SizeT dummyHeaderSize = archiveCounter.cursorPos();

    /**
     * STEP 2 :
     * Dummy serialize to find stream start and stream size for each object
     */
    {
        CBE_PROFILER_SCOPE("DummySerializePackage");

        for (PackageContainedData &containedObjData : containedObjects)
        {
            containedObjData.streamStart = archiveCounter.cursorPos();
            serializeObject(containedObjData.object);
            containedObjData.streamSize = archiveCounter.cursorPos() - containedObjData.streamStart;
            // We must have custom version setup if present, Custom version keys must be from class property name
            containedObjData.classVersion = ArchiveBase::getCustomVersion(uint32(containedObjData.object->getType()->name));
        }
    }

    /**
     * STEP 3 :
     * Copy custom versions and other archive related property to actual packageArchive
     */
    for (const std::pair<const uint32, uint32> &customVersion : ArchiveBase::getCustomVersions())
    {
        packageArchive.setCustomVersion(customVersion.first, customVersion.second);
    }
    packageArchive.setCustomVersion(uint32(PACKAGE_CUSTOM_VERSION_ID), PACKAGE_SERIALIZER_VERSION);

    /**
     * STEP 4 :
     * Now all custom versions and dependent object data are setup we determine new header size
     * Then set stream the final start and size offsets for serialized objects
     */
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

    /**
     * STEP 5 :
     * Setup Array stream to write
     */
    ArrayArchiveStream localStream;
    ArrayArchiveStream *archiveStreamPtr = outStream ? outStream : &localStream;
    archiveStreamPtr->allocate(finalPackageSize);
    packageArchive.setStream(archiveStreamPtr);

    /**
     * STEP 6 :
     * Write into archive
     */
    {
        CBE_PROFILER_SCOPE("SerializePackage");

        (*static_cast<ObjectArchive *>(this)) << *const_cast<StringID *>(&PACKAGE_ARCHIVE_MARKER);
        (*static_cast<ObjectArchive *>(this)) << containedObjects;
        (*static_cast<ObjectArchive *>(this)) << dependentObjects;
        for (PackageContainedData &containedObjData : containedObjects)
        {
            serializeObject(containedObjData.object);
        }
        packageArchive.setStream(nullptr);
    }

    if (outStream == nullptr)
    {
        CBE_PROFILER_SCOPE("PostSavePackage");

        String packagePath = package->getPackageFilePath();
        if (!FileHelper::writeBytes(archiveStreamPtr->getBuffer(), packagePath))
        {
            LOG_ERROR("PackageSaver", "Failed to open file stream to save package {} at {}", package->getObjectData().name, packagePath);
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

    NameString objFullPath = NameString(obj->getObjectData().path);
    auto containedObjItr = objToContObjsIdx.find(objFullPath);
    if (containedObjItr != objToContObjsIdx.end())
    {
        (*static_cast<ObjectArchive *>(this)) << containedObjItr->second;
    }
    else
    {
        auto depObjItr = objToDepObjsIdx.find(objFullPath);
        SizeT depObjIdx = 0;
        // If dependent is not there then we must create a new entry and serialize it
        if (depObjItr == objToDepObjsIdx.end())
        {
            depObjIdx = dependentObjects.size();
            objToDepObjsIdx[objFullPath] = depObjIdx;
            PackageDependencyData &objDepData = dependentObjects.emplace_back();
            objDepData.object = obj;
            objDepData.objectFullPath = objFullPath.toString();
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
