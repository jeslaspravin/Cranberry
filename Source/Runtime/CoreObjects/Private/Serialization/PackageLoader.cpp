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

void PackageLoader::createContainedObject(PackageContainedData &containedData)
{
    String objectName;
    String outerPath;
    ObjectPathHelper::getPathComponents(outerPath, objectName, containedData.objectPath.getChar());

    CBE::Object *outerObj = nullptr;
    if (!outerPath.empty())
    {
        String outerFullPath = package->getName() + ObjectPathHelper::RootObjectSeparator + outerPath;
        outerObj = CBE::get(outerFullPath.getChar());

        alertIf(outerObj, "Outer object being null is unexpected case, Serialization order of objects is outer first to leaf last");
        if (!outerObj)
        {
            auto outerContainedDataItr = std::find_if(
                containedObjects.begin(), containedObjects.end(),
                [&outerPath](const PackageContainedData &containedData) { return containedData.objectPath == outerPath; }
            );

            debugAssert(outerContainedDataItr != containedObjects.end());
            createContainedObject(*outerContainedDataItr);
            outerObj = outerContainedDataItr->object;
        }
    }
    else
    {
        // Empty outer means this is direct child of package
        outerObj = package;
    }

    debugAssert(outerObj);

    CBE::Object *obj = CBE::createOrGet(
        IReflectionRuntimeModule::get()->getClassType(containedData.className), objectName, outerObj, CBE::EObjectFlagBits::PackageLoadPending | containedData.objectFlags
    );
    fatalAssert(obj, "Package(%s) load failed to create object %s", package->getName(), containedData.objectPath);
    containedData.object = obj;
}

PackageLoader::PackageLoader(CBE::Package *loadingPackage, const String &filePath)
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

ObjectArchive &PackageLoader::serialize(CBE::Object *&obj)
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

        if (!CBE::isValid(dependentObjects[tableIdx].object))
        {
            CBE::Object *depObj = CBE::getOrLoad(dependentObjects[tableIdx].objectFullPath);
            fatalAssert(
                depObj && depObj->getType() == IReflectionRuntimeModule::get()->getClassType(dependentObjects[tableIdx].className),
                "Invalid dependent object[%s] in package %s", dependentObjects[tableIdx].objectFullPath, package->getName()
            );
            dependentObjects[tableIdx].object = depObj;
        }
        obj = dependentObjects[tableIdx].object;
    }
    else
    {
        debugAssert(containedObjects.size() > tableIdx && containedObjects[tableIdx].object);
        obj = containedObjects[tableIdx].object;
    }
    return *this;
}

void PackageLoader::prepareLoader()
{
    FileArchiveStream fileStream{ packageFilePath, true };
    fatalAssert(fileStream.isAvailable(), "Package %s at %s cannot be read!", package->getName(), packageFilePath);

    packageArchive.setStream(&fileStream);
    // Set custom versions to this archive to ensure custom versions are available in ObjectArchive
    for (const std::pair<const uint32, uint32> &customVersion : packageArchive.getCustomVersions())
    {
        packageArchive.setCustomVersion(customVersion.first, customVersion.second);
    }

    uint32 packageVersion = getCustomVersion(uint32(PACKAGE_CUSTOM_VERSION_ID));
    fatalAssert(
        packageVersion >= PACKAGE_SERIALIZER_CUTOFF_VERSION, "Package(%s) version %u is not supported. Minimum supported version is %u",
        package->getName(), packageVersion, PACKAGE_SERIALIZER_CUTOFF_VERSION
    );

    (*static_cast<ObjectArchive *>(this)) << containedObjects;
    (*static_cast<ObjectArchive *>(this)) << dependentObjects;
    packageArchive.setStream(nullptr);

    streamStartAt = fileStream.cursorPos();

    alertIf(!containedObjects.empty(), "Empty package %s at %s", package->getName(), packageFilePath);
}

bool PackageLoader::load()
{
    FileArchiveStream fileStream{ packageFilePath, true };
    fatalAssert(fileStream.isAvailable(), "Package %s at %s cannot be read!", package->getName(), packageFilePath);

    packageArchive.setStream(&fileStream);

    bool bSuccess = true;

    // Create all object first
    for (PackageContainedData &containedData : containedObjects)
    {
        if (!CBE::isValid(containedData.object))
        {
            createContainedObject(containedData);
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
        containedData.object->serialize(*this);
        CLEAR_BITS(CBE::INTERNAL_ObjectCoreAccessors::getFlags(containedData.object), CBE::EObjectFlagBits::PackageLoadPending);

        SizeT serializedSize = fileStream.cursorPos() - containedData.streamStart;
        if (serializedSize != containedData.streamSize)
        {
            alertIf(
                serializedSize == containedData.streamSize,
                "Corrupted package %s for object %s consider using Custom version and handle versioning! Written out size for object %llu is "
                "not same as read size %llu",
                package->getName(), containedData.objectPath, containedData.streamSize, (fileStream.cursorPos() - containedData.streamStart)
            );
            bSuccess = false;
        }
    }
    CLEAR_BITS(CBE::INTERNAL_ObjectCoreAccessors::getFlags(package), CBE::EObjectFlagBits::PackageLoadPending);
    return bSuccess;
}
