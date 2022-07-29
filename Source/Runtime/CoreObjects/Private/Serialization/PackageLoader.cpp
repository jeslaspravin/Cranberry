/*!
 * \file PackageLoader.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/PackageLoader.h"
#include "Serialization/FileArchiveStream.h"
#include "Serialization/PackageData.h"
#include "CBEObjectHelpers.h"
#include "CBEPackage.h"
#include "CoreObjectDelegates.h"

EObjectFlags PackageLoader::createContainedObject(PackageContainedData &containedData)
{
    if (containedData.clazz == nullptr)
    {
        containedData.object = nullptr;
        return 0;
    }

    String objectName;
    String outerPath;
    ObjectPathHelper::getPathComponents(outerPath, objectName, containedData.objectPath.getChar());

    EObjectFlags collectedFlags = containedData.objectFlags;
    cbe::Object *outerObj = nullptr;
    if (!outerPath.empty())
    {
        String outerFullPath = package->getName() + ObjectPathHelper::RootObjectSeparator + outerPath;
        outerObj = cbe::get(outerFullPath.getChar());
        if (!outerObj)
        {
            auto outerContainedDataItr = std::find_if(
                containedObjects.begin(), containedObjects.end(),
                [&outerPath](const PackageContainedData &containedData)
                {
                    return containedData.objectPath == outerPath;
                }
            );

            debugAssert(outerContainedDataItr != containedObjects.end());

            collectedFlags |= createContainedObject(*outerContainedDataItr);
            // Transient object can be null
            alertAlwaysf(
                BIT_SET(collectedFlags, cbe::EObjectFlagBits::Transient) || outerObj,
                "Outer object being null is unexpected case, Serialization order of objects is outer first to leaf last"
            );
            outerObj = outerContainedDataItr->object;
        }
        else
        {
            collectedFlags |= outerObj->collectAllFlags();
        }
    }
    else
    {
        // Empty outer means this is direct child of package
        outerObj = package;
        collectedFlags |= outerObj->getFlags();
    }

    if (BIT_SET(collectedFlags, cbe::EObjectFlagBits::Transient))
    {
        // Try to find Transient object. If not found set all pointer fields to nullptr
        if (outerObj == nullptr)
        {
            containedData.object = nullptr;
        }
        else
        {
            cbe::Object *obj = cbe::get(ObjectPathHelper::getFullPath(objectName.getChar(), outerObj).getChar());
            debugAssert(!obj || BIT_SET(obj->collectAllFlags(), cbe::EObjectFlagBits::Transient));
            containedData.object = obj;
        }
    }
    else
    {
        debugAssert(outerObj);
        // It is okay to call createOrGet as we are setting cbe::EObjectFlagBits::PackageLoadPending flags during create itself
        cbe::Object *obj
            = cbe::createOrGet(containedData.clazz, objectName, outerObj, cbe::EObjectFlagBits::PackageLoadPending | containedData.objectFlags);
        alertAlwaysf(obj, "Package(%s) load failed to create object %s", package->getName(), containedData.objectPath);
        containedData.object = obj;
    }
    return collectedFlags;
}

PackageLoader::PackageLoader(cbe::Package *loadingPackage, const String &filePath)
    : package(loadingPackage)
    , packageFilePath(filePath)
{
    debugAssert(package);

    setLoading(true);
    packageArchive.setLoading(true);
    setSwapBytes(false);
    packageArchive.setSwapBytes(false);

    setInnerArchive(&packageArchive);
}

ObjectArchive &PackageLoader::serialize(cbe::Object *&obj)
{
    SizeT tableIdx;
    (*static_cast<ObjectArchive *>(this)) << tableIdx;

    const bool bIsDependent = BIT_SET(tableIdx, DEPENDENT_OBJECT_FLAG);
    CLEAR_BITS(tableIdx, DEPENDENT_OBJECT_FLAG);
    if (tableIdx == NULL_OBJECT_FLAG || (dependentObjects.size() <= tableIdx && containedObjects.size() <= tableIdx))
    {
        obj = nullptr;
        return (*this);
    }

    if (bIsDependent)
    {
        debugAssert(dependentObjects.size() > tableIdx);

        if (!cbe::isValid(dependentObjects[tableIdx].object))
        {
            cbe::Object *depObj = cbe::getOrLoad(dependentObjects[tableIdx].objectFullPath);
            alertAlwaysf(
                depObj && depObj->getType() == dependentObjects[tableIdx].clazz, "Invalid dependent object[%s] in package %s",
                dependentObjects[tableIdx].objectFullPath, package->getName()
            );
            dependentObjects[tableIdx].object = depObj;
        }
        obj = dependentObjects[tableIdx].object;
    }
    else
    {
        debugAssert(containedObjects.size() > tableIdx);
        // Do nothing if no object found
        if (containedObjects[tableIdx].object)
        {
            obj = containedObjects[tableIdx].object;
        }
    }
    return *this;
}

void PackageLoader::prepareLoader()
{
    FileArchiveStream fileStream{ packageFilePath, true };
    fatalAssertf(fileStream.isAvailable(), "Package %s at %s cannot be read!", package->getName(), packageFilePath);

    packageArchive.setStream(&fileStream);
    // Set custom versions to this archive to ensure custom versions are available in ObjectArchive
    for (const std::pair<const uint32, uint32> &customVersion : packageArchive.getCustomVersions())
    {
        setCustomVersion(customVersion.first, customVersion.second);
    }

    uint32 packageVersion = getCustomVersion(uint32(PACKAGE_CUSTOM_VERSION_ID));
    fatalAssertf(
        packageVersion >= PACKAGE_SERIALIZER_CUTOFF_VERSION, "Package(%s) version %u is not supported. Minimum supported version is %u",
        package->getName(), packageVersion, PACKAGE_SERIALIZER_CUTOFF_VERSION
    );

    // Try reading the marker
    {
        StringID packageMarker;
        SizeT packageHeaderStart = fileStream.cursorPos();
        (*static_cast<ObjectArchive *>(this)) << packageMarker;
        if (packageMarker != PACKAGE_ARCHIVE_MARKER)
        {
            LOG_WARN("PackageLoader", "Package marker not found in %s, Trying to load binary stream as marked package!", packageFilePath);
            fileStream.moveBackward(fileStream.cursorPos() - packageHeaderStart);
        }
    }
    (*static_cast<ObjectArchive *>(this)) << containedObjects;
    (*static_cast<ObjectArchive *>(this)) << dependentObjects;
    packageArchive.setStream(nullptr);

    streamStartAt = fileStream.cursorPos();

    alertAlwaysf(!containedObjects.empty(), "Empty package %s at %s", package->getName(), packageFilePath);
    CoreObjectDelegates::broadcastPackageScanned(this);
}

EPackageLoadSaveResult PackageLoader::load()
{
    FileArchiveStream fileStream{ packageFilePath, true };
    if (!fileStream.isAvailable())
    {
        alertAlwaysf(fileStream.isAvailable(), "Package %s at %s cannot be read!", package->getName(), packageFilePath);
        return EPackageLoadSaveResult::IOError;
    }

    packageArchive.setStream(&fileStream);

    EPackageLoadSaveResult loadResult = EPackageLoadSaveResult::Success;

    // Create all object first
    for (PackageContainedData &containedData : containedObjects)
    {
        if (!cbe::isValid(containedData.object))
        {
            // If this object is transient or in transient hierarchy? Then there is a chance that object will only be created after main
            // packaged object is serialized
            EObjectFlags collectedFlags = createContainedObject(containedData);
            debugAssert(BIT_SET(collectedFlags, cbe::EObjectFlagBits::Transient) || cbe::isValid(containedData.object));
        }
    }

    for (PackageContainedData &containedData : containedObjects)
    {
        if (fileStream.cursorPos() > containedData.streamStart)
        {
            fileStream.moveBackward(fileStream.cursorPos() - containedData.streamStart);
        }
        else
        {
            fileStream.moveForward(containedData.streamStart - fileStream.cursorPos());
        }

        // Try loading contained object again if it is created at this point
        if (!cbe::isValid(containedData.object))
        {
            createContainedObject(containedData);
        }
        if (cbe::isValid(containedData.object))
        {
            if (NO_BITS_SET(containedData.object->collectAllFlags(), cbe::EObjectFlagBits::Transient))
            {
                containedData.object->serialize(*this);
                SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(containedData.object), cbe::EObjectFlagBits::PackageLoaded);
            }
            CLEAR_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(containedData.object), cbe::EObjectFlagBits::PackageLoadPending);
        }

        SizeT serializedSize = fileStream.cursorPos() - containedData.streamStart;
        if (serializedSize != containedData.streamSize)
        {
            alertAlwaysf(
                serializedSize == containedData.streamSize,
                "Corrupted package %s for object %s consider using Custom version and handle versioning! Written out size for object %llu is "
                "not same as read size %llu",
                package->getName(), containedData.objectPath, containedData.streamSize, (fileStream.cursorPos() - containedData.streamStart)
            );
            // It is okay to continue as it is just warning
            loadResult = EPackageLoadSaveResult::WithWarnings;
        }
    }
    CLEAR_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::PackageLoadPending);
    SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::PackageLoaded);

    // Broadcast load events, postLoad() and constructed()
    for (PackageContainedData &containedData : containedObjects)
    {
        if (cbe::isValid(containedData.object))
        {
            containedData.object->postLoad();
        }
    }
    CoreObjectDelegates::broadcastPackageLoaded(package);
    for (PackageContainedData &containedData : containedObjects)
    {
        if (cbe::isValid(containedData.object))
        {
            containedData.object->constructed();
        }
    }
    package->constructed();

    return loadResult;
}
