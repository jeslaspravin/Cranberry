/*!
 * \file ObjectSerializationHelpers.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/ObjectSerializationHelpers.h"
#include "Property/CustomProperty.h"
#include "Visitors/FieldVisitors.h"

using FieldSizeDataType = SizeT;

inline constexpr static const uint32 OBJECTFIELD_SER_VERSION = 0;
inline constexpr static const uint32 OBJECTFIELD_SER_CUTOFF_VERSION = 0;
inline STRINGID_CONSTEXPR static const StringID OBJECTFIELD_SER_CUSTOM_VERSION_ID = STRID("ObjectFieldsSerializer");

//////////////////////////////////////////////////////////////////////////
// Reading visitors
//////////////////////////////////////////////////////////////////////////

struct ReadObjectFieldUserData
{
    ObjectArchive *ar;
    // End cursor to ensure that read does not go too mush beyond its end limit
    SizeT fieldEndCursor;
    std::vector<uint8> scratchPad;
};

template <typename Type>
concept UnsupportedFundamentalOrSpecial = std::is_const_v<Type> || std::is_pointer_v<Type> || std::is_reference_v<Type>();

struct ReadFieldVisitable
{
    // Ignore const type
    template <UnsupportedFundamentalOrSpecial Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        alertIf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
    }

    // above UnsupportedFundamentalOrSpecial takes precedence over below generic support
    template <typename Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        ReadObjectFieldUserData *readUserData = (ReadObjectFieldUserData *)(userData);
        if (readUserData->fieldEndCursor > readUserData->ar->stream()->cursorPos())
        {
            (*readUserData->ar) << (*val);
        }
    }
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        ReadObjectFieldUserData *readUserData = (ReadObjectFieldUserData *)(userData);
        
        // If we are already over the limit skip serializing
        if (readUserData->fieldEndCursor <= readUserData->ar->stream()->cursorPos())
        {
            return;
        }

        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            const MapProperty *mapProp = static_cast<const MapProperty *>(prop);
            const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(mapProp->dataRetriever);

            // Do not use element property here as it has possibility of being null when pair data type is not generated some where else
            const TypedProperty *keyProp = static_cast<const TypedProperty *>(mapProp->keyProp);
            const TypedProperty *valueProp = static_cast<const TypedProperty *>(mapProp->valueProp);

            // Each pair's data for inserting into this map
            readUserData->scratchPad.resize(mapProp->pairSize);
            // This is to ensure that key, value visitor do not overwrite the outer scratchPad data. Temp scratch data for key, value
            // serialization
            ReadObjectFieldUserData newUserData{ *readUserData };

            SizeT elementCount = 0;
            (*readUserData->ar) << elementCount;
            for (SizeT i = 0; i < elementCount; ++i)
            {
                // If element count is read from invalid binary stream, Then we must not cross end cursor
                if (readUserData->fieldEndCursor <= readUserData->ar->stream()->cursorPos())
                {
                    break;
                }
                // zero and reconstruct for each element to avoid using previous values
                CBEMemory::memZero(readUserData->scratchPad.data(), readUserData->scratchPad.size());
                dataRetriever->contruct(readUserData->scratchPad.data());

                FieldVisitor::visit<ReadFieldVisitable>(keyProp, readUserData->scratchPad.data(), &newUserData);
                FieldVisitor::visit<ReadFieldVisitable>(valueProp, readUserData->scratchPad.data() + mapProp->secondOffset, &newUserData);
                dataRetriever->add(val, readUserData->scratchPad.data(), true);
            }
            break;
        }
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        {
            const IterateableDataRetriever *dataRetriever
                = static_cast<const IterateableDataRetriever *>(static_cast<const ContainerProperty *>(prop)->dataRetriever);
            const TypedProperty *elemProp = static_cast<const TypedProperty *>(static_cast<const ContainerProperty *>(prop)->elementProp);

            // Each element data for inserting into this map
            readUserData->scratchPad.resize(elemProp->typeInfo->size);
            // This is to ensure that element visitor do not overwrite the outer scratchPad data, Temp scratch data for element serialization
            ReadObjectFieldUserData newUserData{ *readUserData };

            SizeT containerSize = 0;
            (*readUserData->ar) << containerSize;
            for (SizeT i = 0; i < containerSize; ++i)
            {
                // If element count is read from invalid binary stream, Then we must not cross end cursor
                if (readUserData->fieldEndCursor <= readUserData->ar->stream()->cursorPos())
                {
                    break;
                }
                // zero and reconstruct for each element to avoid using previous values
                CBEMemory::memZero(readUserData->scratchPad.data(), readUserData->scratchPad.size());
                dataRetriever->contruct(readUserData->scratchPad.data());

                FieldVisitor::visit<ReadFieldVisitable>(elemProp, readUserData->scratchPad.data(), &newUserData);
                dataRetriever->add(val, readUserData->scratchPad.data(), true);
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

            FieldVisitor::visit<ReadFieldVisitable>(keyProp, keyPtr, userData);
            FieldVisitor::visit<ReadFieldVisitable>(valueProp, valPtr, userData);
            break;
        }
        case EPropertyType::ClassType:
        {
            CBEClass clazz = static_cast<CBEClass>(prop);
            debugAssert(PropertyHelper::isStruct(clazz));
            FieldVisitor::visitFields<ReadFieldVisitable>(clazz, val, userData);
            break;
        }
        case EPropertyType::EnumType:
        {
            const EnumProperty *enumProp = static_cast<const EnumProperty *>(propInfo.thisProperty);
            if (enumProp->fields.empty())
            {
                break;
            }

            SizeT enumVal;
            (*readUserData->ar) << enumVal;
            PropertyHelper::setValidEnumValue(val, enumVal, enumProp);
            break;
        }
        }
    }
    // Ignoring const types
    static void visit(const void *val, const PropertyInfo &propInfo, void *userData)
    {
        alertIf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
    }
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        ReadObjectFieldUserData *readUserData = (ReadObjectFieldUserData *)(userData);
        // If we are already over the limit skip serializing
        if (readUserData->fieldEndCursor <= readUserData->ar->stream()->cursorPos())
        {
            return;
        }

        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), CBE::Object::staticType()));

            CBE::Object **objPtrPtr = reinterpret_cast<CBE::Object **>(ptr);
            (*readUserData->ar) << (*objPtrPtr);
            break;
        }
        case EPropertyType::EnumType:
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        default:
            alertIf(false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo);
            break;
        }
    }
    // It is okay we are not going to do anything that violates constant
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData) { visit(const_cast<void **>(ptr), propInfo, userData); }
};

//////////////////////////////////////////////////////////////////////////
// Writing visitors
//////////////////////////////////////////////////////////////////////////

struct WriteObjectFieldUserData
{
    ObjectArchive *ar;
};

struct WriteFieldVisitable
{
    // Ignore const type
    template <UnsupportedFundamentalOrSpecial Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        alertIf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
    }

    // above UnsupportedFundamentalOrSpecial takes precedence over below generic support
    template <typename Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        WriteObjectFieldUserData *writeUserData = (WriteObjectFieldUserData *)(userData);
        (*writeUserData->ar) << (*val);
    }
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        WriteObjectFieldUserData *writeUserData = (WriteObjectFieldUserData *)(userData);

        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            const IterateableDataRetriever *dataRetriever
                = static_cast<const IterateableDataRetriever *>(static_cast<const MapProperty *>(prop)->dataRetriever);

            // Do not use element property here as it has possibility of being null when pair data type is not generated some where else
            const TypedProperty *keyProp = static_cast<const TypedProperty *>(static_cast<const MapProperty *>(prop)->keyProp);
            const TypedProperty *valueProp = static_cast<const TypedProperty *>(static_cast<const MapProperty *>(prop)->valueProp);

            SizeT elementCount = dataRetriever->size(val);
            (*writeUserData->ar) << elementCount;
            for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
            {
                FieldVisitor::visit<WriteFieldVisitable>(keyProp, itrPtr->getElement(), userData);
                FieldVisitor::visit<WriteFieldVisitable>(valueProp, static_cast<MapIteratorWrapper *>(itrPtr.get())->value(), userData);
            }
            break;
        }
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        {
            const IterateableDataRetriever *dataRetriever
                = static_cast<const IterateableDataRetriever *>(static_cast<const ContainerProperty *>(prop)->dataRetriever);
            const TypedProperty *elemProp = static_cast<const TypedProperty *>(static_cast<const ContainerProperty *>(prop)->elementProp);

            SizeT containerSize = dataRetriever->size(val);
            (*writeUserData->ar) << containerSize;
            for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
            {
                FieldVisitor::visit<WriteFieldVisitable>(elemProp, itrPtr->getElement(), userData);
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

            FieldVisitor::visit<WriteFieldVisitable>(keyProp, keyPtr, userData);
            FieldVisitor::visit<WriteFieldVisitable>(valueProp, valPtr, userData);
            break;
        }
        case EPropertyType::ClassType:
        {
            CBEClass clazz = static_cast<CBEClass>(prop);
            debugAssert(PropertyHelper::isStruct(clazz));
            FieldVisitor::visitFields<WriteFieldVisitable>(clazz, val, userData);
            break;
        }
        case EPropertyType::EnumType:
        {
            const EnumProperty *enumProp = static_cast<const EnumProperty *>(propInfo.thisProperty);
            if (enumProp->fields.empty())
            {
                break;
            }

            SizeT enumVal = PropertyHelper::getValidEnumValue(val, enumProp);
            (*writeUserData->ar) << enumVal;
            break;
        }
        }
    }
    // Ignoring const types
    static void visit(const void *val, const PropertyInfo &propInfo, void *userData)
    {
        alertIf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
    }
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), CBE::Object::staticType()));

            WriteObjectFieldUserData *writeUserData = (WriteObjectFieldUserData *)(userData);
            CBE::Object **objPtrPtr = reinterpret_cast<CBE::Object **>(ptr);
            (*writeUserData->ar) << (*objPtrPtr);
            break;
        }
        case EPropertyType::EnumType:
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        default:
            alertIf(false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo);
            break;
        }
    }
    // It is okay we are not going to do anything that violates constant
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData) { visit(const_cast<void **>(ptr), propInfo, userData); }
};

/**
 * Serializes each field such that for a field
 *
 * 1st serializes field's name id
 *
 * 2nd serializes data size for this field in bytes. So that stream can be offset to next field in case of field's type changed(Of course this
 * field will be corrupted but that is okay as other fields will be fine)
 *
 * 3rd serialized field's data itself
 */
struct StartWriteFieldVisitable
{
    // Ignore const type
    template <typename Type>
    requires std::is_const_v<Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData) {}
    template <typename Type>
    requires(!std::is_const_v<Type>) static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        debugAssert(propInfo.fieldProperty);
        WriteObjectFieldUserData *writeUserData = (WriteObjectFieldUserData *)(userData);

        StringID fieldNameId = propInfo.fieldProperty->name;
        (*writeUserData->ar) << fieldNameId;
        // We do not know size yet so just skip now and fill later
        writeUserData->ar->stream()->moveForward(sizeof(FieldSizeDataType));
        SizeT dataStartCursor = writeUserData->ar->stream()->cursorPos();

        WriteFieldVisitable::visit(val, propInfo, userData);

        FieldSizeDataType dataSize = (FieldSizeDataType)(writeUserData->ar->stream()->cursorPos() - dataStartCursor);
        writeUserData->ar->stream()->moveBackward(dataSize + sizeof(FieldSizeDataType));
        (*writeUserData->ar) << dataSize;
        writeUserData->ar->stream()->moveForward(dataSize);
    }

    // It is okay we are not going to do anything that violates constant
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData) { visit(const_cast<void **>(ptr), propInfo, userData); }
};

//////////////////////////////////////////////////////////////////////////
// Helper implementations
//////////////////////////////////////////////////////////////////////////

ObjectArchive &ObjectSerializationHelpers::serializeAllFields(CBE::Object *obj, ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 objectFieldSerVersion = ar.getCustomVersion((uint32)(OBJECTFIELD_SER_CUSTOM_VERSION_ID));
        fatalAssert(
            objectFieldSerVersion >= OBJECTFIELD_SER_CUTOFF_VERSION,
            "Unsupport version %d of serialized object fields of object %s! Minimum supported version %d", objectFieldSerVersion,
            obj->getFullPath(), OBJECTFIELD_SER_CUTOFF_VERSION
        );

        ReadObjectFieldUserData userData{ .ar = &ar };
        while (ar.stream()->hasMoreData(sizeof(StringID::IDType)))
        {
            StringID fieldNameId;
            FieldSizeDataType fieldDataSize;
            ar << fieldNameId;
            // Invalid StringID is used to mark end of all serialized fields for this object
            if (fieldNameId == StringID::INVALID)
            {
                break;
            }
            ar << fieldDataSize;

            SizeT dataStartCursor = ar.stream()->cursorPos();
            userData.fieldEndCursor = dataStartCursor + fieldDataSize;
            if (const FieldProperty *fieldProp = PropertyHelper::findField(obj->getType(), fieldNameId))
            {
                void *val = static_cast<const MemberFieldWrapper *>(fieldProp->fieldPtr)->get(obj);
                FieldVisitor::visit<ReadFieldVisitable>(static_cast<const TypedProperty *>(fieldProp->field), val, &userData);
            }

            // Why would archive stream be moving backward?
            debugAssert(ar.stream()->cursorPos() >= dataStartCursor);
            // Moving cursor back to end of this field so next field can be serialized. 
            // Not using already calculated fieldEndCursor as new cursor might end up less that fieldEndCursor which is against above assert
            ar.stream()->moveBackward(ar.stream()->cursorPos() - dataStartCursor);
            ar.stream()->moveForward(fieldDataSize);
        }
    }
    else
    {
        ar.setCustomVersion(uint32(OBJECTFIELD_SER_CUSTOM_VERSION_ID), OBJECTFIELD_SER_VERSION);

        WriteObjectFieldUserData userData{ .ar = &ar };
        FieldVisitor::visitFields<StartWriteFieldVisitable>(obj->getType(), obj, &userData);
        // Append an invalid StringID to make finding end of fields
        StringID invalidID = StringID::INVALID;
        ar << invalidID;
    }
    return ar;
}