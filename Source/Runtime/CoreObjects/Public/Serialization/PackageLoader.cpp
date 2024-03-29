/*!
 * \file PackageLoader.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/LFS/File/FileHelper.h"
#include "Serialization/PackageLoader.h"
#include "Serialization/ArrayArchiveStream.h"
#include "Serialization/PackageData.h"
#include "Visitors/FieldVisitors.h"
#include "PropertyVisitorHelpers.h"
#include "CoreObjectsModule.h"
#include "CBEObjectHelpers.h"
#include "CBEPackage.h"
#include "CoreObjectDelegates.h"

//////////////////////////////////////////////////////////////////////////
// Object Pointers relinking codes
//////////////////////////////////////////////////////////////////////////

struct LinkObjectPtrsData
{
    const PackageLoader *loader;
};

struct LinkObjPtrsFieldVisitable
{
    // Ignore fundamental and special types, we need none const custom types or pointers
    template <typename Type>
    static void visit(Type *, const PropertyInfo &, void *)
    {}
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            PropertyVisitorHelper::visitEditMapEntriesPtrOnly<LinkObjPtrsFieldVisitable>(
                static_cast<const MapProperty *>(prop), val, propInfo, userData
            );
            break;
        }
        case EPropertyType::SetType:
        {
            PropertyVisitorHelper::visitEditSetEntries<LinkObjPtrsFieldVisitable>(
                static_cast<const ContainerProperty *>(prop), val, propInfo, userData
            );
            break;
        }
        case EPropertyType::ArrayType:
        {
            const IterateableDataRetriever *dataRetriever
                = static_cast<const IterateableDataRetriever *>(static_cast<const ContainerProperty *>(prop)->dataRetriever);
            const TypedProperty *elemProp = static_cast<const TypedProperty *>(static_cast<const ContainerProperty *>(prop)->elementProp);

            for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
            {
                FieldVisitor::visit<LinkObjPtrsFieldVisitable>(elemProp, itrPtr->getElement(), userData);
            }
            break;
        }
        case EPropertyType::PairType:
        {
            const PairDataRetriever *dataRetriever
                = static_cast<const PairDataRetriever *>(static_cast<const PairProperty *>(prop)->dataRetriever);
            const TypedProperty *keyProp = static_cast<const TypedProperty *>(static_cast<const PairProperty *>(prop)->keyProp);
            const TypedProperty *valueProp = static_cast<const TypedProperty *>(static_cast<const PairProperty *>(prop)->valueProp);

            void *keyPtr = dataRetriever->first(val);
            void *valPtr = dataRetriever->second(val);

            FieldVisitor::visit<LinkObjPtrsFieldVisitable>(keyProp, keyPtr, userData);
            FieldVisitor::visit<LinkObjPtrsFieldVisitable>(valueProp, valPtr, userData);
            break;
        }
        case EPropertyType::ClassType:
        {
            CBEClass clazz = static_cast<CBEClass>(prop);
            debugAssert(PropertyHelper::isStruct(clazz));
            FieldVisitor::visitFields<LinkObjPtrsFieldVisitable>(clazz, val, userData);
            break;
        }
        case EPropertyType::EnumType:
            break;
        }
    }
    // Ignoring const types
    static void visit(const void *, const PropertyInfo &, void *) {}
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            LinkObjectPtrsData *linkPtrsUserData = (LinkObjectPtrsData *)(userData);
            linkPtrsUserData->loader->relinkSerializedPtr(ptr);
            break;
        }
        case EPropertyType::EnumType:
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        default:
            alertAlwaysf(
                false, "Unhandled ptr to ptr Field name {}, type {}", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
            );
            break;
        }
    }
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (propInfo.thisProperty->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            LinkObjectPtrsData *linkPtrsUserData = (LinkObjectPtrsData *)(userData);
            linkPtrsUserData->loader->relinkSerializedPtr(ptr);
            break;
        }
        case EPropertyType::EnumType:
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        default:
            alertAlwaysf(
                false, "Unhandled ptr to const ptr Field name {}, type {}", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
            );
            break;
        }
    }
};

FORCE_INLINE void PackageLoader::linkContainedObjects() const
{
    if (!bDelayLinkRequired)
    {
        return;
    }

    CBE_PROFILER_SCOPE("LinkPackageObjRefs");

    LinkObjectPtrsData userData{ this };
    for (const PackageContainedData &containedData : containedObjects)
    {
        if (containedData.object.isValid())
        {
            FieldVisitor::visitFields<LinkObjPtrsFieldVisitable>(containedData.clazz, containedData.object.get(), &userData);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// PackageLoader specific implementations
//////////////////////////////////////////////////////////////////////////

EObjectFlags PackageLoader::createContainedObject(PackageContainedData &containedData, const String &packageName, EObjectFlags packageFlags)
{
    if (containedData.clazz == nullptr)
    {
        containedData.object = nullptr;
        return 0;
    }

    StringView objectName;
    StringView outerPath;
    ObjectPathHelper::getPathComponents(outerPath, objectName, containedData.objectPath.getChar());

    EObjectFlags collectedFlags = containedData.objectFlags;
    cbe::Object *outerObj = nullptr;
    if (!outerPath.empty())
    {
        String outerFullPath = (packageName + ObjectPathHelper::RootObjectSeparator).append(outerPath);
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

            collectedFlags |= createContainedObject(*outerContainedDataItr, packageName, packageFlags);
            // Transient object can be null
            alertAlwaysf(
                BIT_SET(collectedFlags, cbe::EObjectFlagBits::ObjFlag_Transient) || outerObj,
                "Outer object being null is unexpected case, Serialization order of objects is outer first to leaf last"
            );
            outerObj = outerContainedDataItr->object.get();
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
        collectedFlags |= packageFlags;
    }

    if (BIT_SET(collectedFlags, cbe::EObjectFlagBits::ObjFlag_Transient))
    {
        // Try to find Transient object. If not found set all pointer fields to nullptr
        if (outerObj == nullptr)
        {
            containedData.object = nullptr;
        }
        else
        {
            cbe::Object *obj = cbe::get(ObjectPathHelper::getFullPath(objectName, outerObj).getChar());
            debugAssert(!obj || BIT_SET(obj->collectAllFlags(), cbe::EObjectFlagBits::ObjFlag_Transient));
            containedData.object = obj;
        }
    }
    else
    {
        debugAssert(outerObj);
        // It is okay to call createOrGet as we are setting cbe::EObjectFlagBits::PackageLoadPending flags during create itself
        cbe::Object *obj = cbe::createOrGet(
            containedData.clazz, objectName, outerObj, cbe::EObjectFlagBits::ObjFlag_PackageLoadPending | containedData.objectFlags
        );
        alertAlwaysf(obj, "Package({}) load failed to create object {}", packageName, containedData.objectPath);
        containedData.object = obj;
    }
    return collectedFlags;
}

template <typename T>
FORCE_INLINE void PackageLoader::relinkLoadedPtr(T **objPtrPtr) const
{
    UPtrInt *ptrIntPtr = reinterpret_cast<UPtrInt *>(objPtrPtr);
    if (BIT_SET(*ptrIntPtr, delayLinkPtrMask))
    {
        CLEAR_BITS(*ptrIntPtr, delayLinkPtrMask);

        debugAssert(containedObjects.size() > *ptrIntPtr);
        if (containedObjects[*ptrIntPtr].object.isValid())
        {
            *objPtrPtr = containedObjects[*ptrIntPtr].object.get();
        }
        else
        {
            *objPtrPtr = nullptr;
        }
    }
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

void PackageLoader::relinkSerializedPtr(void **objPtrPtr) const { relinkLoadedPtr(objPtrPtr); }

void PackageLoader::relinkSerializedPtr(const void **objPtrPtr) const { relinkLoadedPtr(objPtrPtr); }

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

        if (!dependentObjects[tableIdx].object.isValid())
        {
            cbe::Object *depObj = cbe::getOrLoad(dependentObjects[tableIdx].objectFullPath, dependentObjects[tableIdx].clazz);
            alertAlwaysf(
                depObj, "Invalid dependent object[{}] in package {}", dependentObjects[tableIdx].objectFullPath, package->getObjectData().name
            );
            dependentObjects[tableIdx].object = depObj;
        }
        obj = dependentObjects[tableIdx].object.get();
    }
    else
    {
        debugAssert(containedObjects.size() > tableIdx);
        // Add to delayed linking if no object found
        if (containedObjects[tableIdx].object.isValid())
        {
            obj = containedObjects[tableIdx].object.get();
        }
        else
        {
            // Set ptr value as SENTINEL_LINK_PTR with lsb bits replaced with containedObjects index(tableIdx)
            // This will be later replaced with actual value at relinkSerializedPtr(ptr)
            UPtrInt *objPtrPtr = reinterpret_cast<UPtrInt *>(&obj);
            *objPtrPtr = delayLinkPtrMask + tableIdx;
            bDelayLinkRequired = true;
        }
    }
    return *this;
}

void PackageLoader::prepareLoader()
{
    cbe::ObjectPrivateDataView packageDatV = package->getObjectData();

    ArrayArchiveStream localStream;
    ArrayArchiveStream *archiveStreamPtr = inStream;
    if (inStream == nullptr)
    {
        std::vector<uint8> fileData;
        bool bRead = FileHelper::readBytes(fileData, packageFilePath);
        fatalAssertf(bRead, "Package {} at {} cannot be read!", packageDatV.name, packageFilePath);
        localStream.setBuffer(fileData);
        archiveStreamPtr = &localStream;
    }

    packageArchive.setStream(&localStream);
    // Set custom versions to this archive to ensure custom versions are available in ObjectArchive
    for (const std::pair<const uint32, uint32> &customVersion : packageArchive.getCustomVersions())
    {
        setCustomVersion(customVersion.first, customVersion.second);
    }

    uint32 packageVersion = getCustomVersion(uint32(PACKAGE_CUSTOM_VERSION_ID));
    fatalAssertf(
        packageVersion >= PACKAGE_SERIALIZER_CUTOFF_VERSION, "Package({}) version {} is not supported. Minimum supported version is {}",
        packageDatV.name, packageVersion, PACKAGE_SERIALIZER_CUTOFF_VERSION
    );

    // Try reading the marker
    {
        StringID packageMarker;
        SizeT packageHeaderStart = archiveStreamPtr->cursorPos();
        (*static_cast<ObjectArchive *>(this)) << packageMarker;
        if (packageMarker != PACKAGE_ARCHIVE_MARKER)
        {
            LOG_WARN("PackageLoader", "Package marker not found in {}, Trying to load binary stream as marked package!", packageFilePath);
            archiveStreamPtr->moveBackward(archiveStreamPtr->cursorPos() - packageHeaderStart);
        }
    }
    (*static_cast<ObjectArchive *>(this)) << containedObjects;
    (*static_cast<ObjectArchive *>(this)) << dependentObjects;
    packageArchive.setStream(nullptr);

    // Mask exact bits that are necessary for adding containedObjectIdx
    delayLinkPtrMask = SENTINEL_LINK_PTR;
    UPtrInt clearSentinelBits = Math::toHigherPowOf2(containedObjects.size()) - 1;
    debugAssert(BIT_SET(clearSentinelBits, containedObjects.size() - 1));
    CLEAR_BITS(delayLinkPtrMask, clearSentinelBits);

    streamStartAt = archiveStreamPtr->cursorPos();

    alertAlwaysf(!containedObjects.empty(), "Empty package {} at {}", packageDatV.name, packageFilePath);
    CoreObjectDelegates::broadcastPackageScanned(this);
}

EPackageLoadSaveResult PackageLoader::load()
{
    // Do not use this
    cbe::ObjectPrivateDataView tempPackageDatV = package->getObjectData();
    // This is temporary flag cache to avoid getting package object data for every createContainedObject().
    EObjectFlags packageFlag = tempPackageDatV.flags;
    String packageName = tempPackageDatV.name;
    tempPackageDatV = {};

    ArrayArchiveStream localStream;
    ArrayArchiveStream *archiveStreamPtr = inStream;
    if (inStream == nullptr)
    {
        CBE_PROFILER_SCOPE("ReadPackageArchive");

        std::vector<uint8> fileData;
        bool bRead = FileHelper::readBytes(fileData, packageFilePath);
        if (!bRead)
        {
            alertAlwaysf(bRead, "Package {} at {} cannot be read!", packageName, packageFilePath);
            return EPackageLoadSaveResult::IOError;
        }
        localStream.setBuffer(fileData);
        archiveStreamPtr = &localStream;
    }

    packageArchive.setStream(archiveStreamPtr);

    EPackageLoadSaveResult loadResult = EPackageLoadSaveResult::Success;
    auto containedObjSerializer = [&loadResult, &packageName, archiveStreamPtr, this](PackageContainedData &containedData)
    {
        CBE_PROFILER_SCOPE("SerializeObj");

        if (archiveStreamPtr->cursorPos() > containedData.streamStart)
        {
            archiveStreamPtr->moveBackward(archiveStreamPtr->cursorPos() - containedData.streamStart);
        }
        else
        {
            archiveStreamPtr->moveForward(containedData.streamStart - archiveStreamPtr->cursorPos());
        }

        if (containedData.object.isValid()
            && BIT_SET(containedData.object->getObjectData().flags, cbe::EObjectFlagBits::ObjFlag_PackageLoadPending))
        {
            if (NO_BITS_SET(containedData.object->collectAllFlags(), cbe::EObjectFlagBits::ObjFlag_Transient))
            {
                containedData.object->serialize(*this);
                SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(containedData.object.get()), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);
            }
            CLEAR_BITS(
                cbe::INTERNAL_ObjectCoreAccessors::getFlags(containedData.object.get()), cbe::EObjectFlagBits::ObjFlag_PackageLoadPending
            );

            // Check serialized size to ensure we are matching that was saved
            SizeT serializedSize = archiveStreamPtr->cursorPos() - containedData.streamStart;
            if (serializedSize != containedData.streamSize)
            {
                alertAlwaysf(
                    serializedSize == containedData.streamSize,
                    "Corrupted package {} for object {} consider using Custom version and handle versioning! Written out size for object {} "
                    "is "
                    "not same as read size {}",
                    packageName, containedData.objectPath, containedData.streamSize, (archiveStreamPtr->cursorPos() - containedData.streamStart)
                );
                // It is okay to continue as it is just warning
                loadResult = EPackageLoadSaveResult::WithWarnings;
            }
        }
    };

    {
        CBE_PROFILER_SCOPE("CreatePackageObjs");

        // Create all object first
        for (PackageContainedData &containedData : containedObjects)
        {
            if (!containedData.object.isValid())
            {
                // If this object is transient or in transient hierarchy? Then there is a chance that object will only be created after main
                // packaged object is serialized
                EObjectFlags collectedFlags = createContainedObject(containedData, packageName, packageFlag);
                debugAssert(BIT_SET(collectedFlags, cbe::EObjectFlagBits::ObjFlag_Transient) || containedData.object.isValid());
            }
        }
    }

    // Load each object. Transient objects might not have been linked yet.
    for (PackageContainedData &containedData : containedObjects)
    {
        containedObjSerializer(containedData);
    }
    // Try caching the possibly created transient containedObjects again
    for (PackageContainedData &containedData : containedObjects)
    {
        if (!containedData.object.isValid())
        {
            createContainedObject(containedData, packageName, packageFlag);
        }
    }
    // Now link the pointers that points to delay created objects
    linkContainedObjects();

    // Broadcast post serialize event
    {
        CBE_PROFILER_SCOPE("PostSerializePackage");

        for (PackageContainedData &containedData : containedObjects)
        {
            if (containedData.object.isValid())
            {
                containedData.object->postSerialize(*this);
            }
        }
    }

    CLEAR_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::ObjFlag_PackageLoadPending);
    SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);

    // Broadcast load events, postLoad() and constructed()
    {
        CBE_PROFILER_SCOPE("PostLoadPackage");

        for (PackageContainedData &containedData : containedObjects)
        {
            if (containedData.object.isValid())
            {
                containedData.object->postLoad();
            }
        }
        CoreObjectDelegates::broadcastPackageLoaded(package);
    }
    {
        CBE_PROFILER_SCOPE("ConstructedPackage");

        for (PackageContainedData &containedData : containedObjects)
        {
            if (containedData.object.isValid())
            {
                containedData.object->constructed();
            }
        }
        package->constructed();
    }

    return loadResult;
}

void PackageLoader::unload()
{
    SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::ObjFlag_PackageLoadPending);
    CLEAR_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(package), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);
    for (PackageContainedData &containedData : containedObjects)
    {
        if (cbe::Object *obj = containedData.object.get())
        {
            SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(obj), cbe::EObjectFlagBits::ObjFlag_PackageLoadPending);
            CLEAR_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(obj), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);
        }
        else
        {
            containedData.object.reset();
        }
    }
    CoreObjectDelegates::broadcastPackageUnloaded(package);
}