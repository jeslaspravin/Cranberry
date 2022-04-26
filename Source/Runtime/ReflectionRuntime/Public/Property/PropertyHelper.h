/*!
 * \file PropertyHelper.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "IReflectionRuntime.h"
#include "Property/Property.h"
#include "ReflectionRuntimeExports.h"
#include "String/String.h"
#include "Types/FunctionTypes.h"

class ClassProperty;

template <typename Type>
concept ReflectClassOrStructType = requires
{
    {
        Type::staticType()
        } -> std::same_as<const ClassProperty *>;
};
template <typename Type>
concept ReflectClassType = requires(Type *object)
{
    {
        Type::staticType()
        } -> std::same_as<const ClassProperty *>;
    {
        object->getType()
        } -> std::same_as<const ClassProperty *>;
};

// Has to negate ReflectedClassType possibility as class inheriting interface will inherit same interface structure
template <typename Type>
concept InterfaceType
    = std::same_as<typename Type::GENERATED_INTERFACE_CODES_ALIAS, uint32> && !ReflectClassType<Type> && requires(Type * interfaceObj)
{
    {
        interfaceObj->getType()
        } -> std::same_as<const ClassProperty *>;
};

template <typename Type>
concept ReflectClassOrStructOrInterfaceType = ReflectClassOrStructType<Type> || InterfaceType<Type>;

#define VALID_SYMBOL_REGEX_PATTERN TCHAR("^[a-zA-Z_]{1}[a-zA-Z0-9_]*")
class REFLECTIONRUNTIME_EXPORT PropertyHelper
{
private:
    PropertyHelper() = default;

public:
    /**
     * PropertyHelper::getValidSymbolName -
     * Returns valid name that be used as name for function or class or other user symbol in C++.
     * Does not handles special key words in C++ so caller must ensure that resulting string does not
     * leads to keyword Replaces any none allowed characters in symbol name to underscore
     *
     * Access: public static
     *
     * @param const String & inValue
     *
     * @return String
     */
    static String getValidSymbolName(const String &inValue);
    static bool isValidSymbolName(const String &inValue);
    static bool isValidFunctionCall(const String &inValue);

    FORCE_INLINE static bool isMapType(const String &typeName)
    {
        return typeName.startsWith(TCHAR("std::map")) || typeName.startsWith(TCHAR("std::unordered_map"));
    }
    FORCE_INLINE static bool isPairType(const String &typeName) { return typeName.startsWith(TCHAR("std::pair")); }
    FORCE_INLINE static bool isSetType(const String &typeName)
    {
        return typeName.startsWith(TCHAR("std::set")) || typeName.startsWith(TCHAR("std::unordered_set"));
    }
    FORCE_INLINE static bool isArrayType(const String &typeName) { return typeName.startsWith(TCHAR("std::vector")); }

    FORCE_INLINE static const TypedProperty *getUnqualified(const BaseProperty *prop)
    {
        return static_cast<const TypedProperty *>(
            prop->type == EPropertyType::QualifiedType ? static_cast<const QualifiedProperty *>(prop)->unqualTypeProperty : prop
        );
    }

    FORCE_INLINE static bool isValidEnumValue(SizeT value, const EnumProperty *enumProp)
    {
        if (enumProp->fields.empty())
        {
            return value == 0;
        }

        bool bIsValid = false;
        for (const EnumProperty::EnumField &entry : enumProp->fields)
        {
            bIsValid = bIsValid || (entry.value == value);
        }
        return bIsValid;
    }
    // Returns true if all bits in enum is valid flag
    FORCE_INLINE static bool isValidEnumFlags(SizeT value, const EnumProperty *enumProp)
    {
        if (!enumProp->bIsFlags)
        {
            return isValidEnumValue(value, enumProp);
        }

        // Must be zero after clearing all valid bits
        SizeT allValidBits = 0;
        for (const EnumProperty::EnumField &entry : enumProp->fields)
        {
            SET_BITS(allValidBits, entry.value);
        }
        CLEAR_BITS(value, allValidBits);
        return (value == 0);
    }
    // Clears invalid value and makes it valid, If flags then clears invalid flags only
    FORCE_INLINE static SizeT clearInvalidEnumValues(SizeT value, const EnumProperty *enumProp)
    {
        if (!enumProp->bIsFlags)
        {
            return isValidEnumValue(value, enumProp) ? value : enumProp->fields.empty() ? 0 : enumProp->fields[0].value;
        }

        SizeT allValidBits = 0;
        for (const EnumProperty::EnumField &entry : enumProp->fields)
        {
            SET_BITS(allValidBits, entry.value);
        }
        return value & allValidBits;
    }
    FORCE_INLINE static SizeT getValidEnumValue(void *val, const EnumProperty *enumProp)
    {
        SizeT enumVal = 0;
        if (enumProp->typeInfo->size == 1)
        {
            enumVal = *reinterpret_cast<uint8 *>(val);
        }
        else if (enumProp->typeInfo->size == 2)
        {
            enumVal = *reinterpret_cast<uint16 *>(val);
        }
        else if (enumProp->typeInfo->size == 4)
        {
            enumVal = *reinterpret_cast<uint32 *>(val);
        }
        else if (enumProp->typeInfo->size == 8)
        {
            enumVal = *reinterpret_cast<uint64 *>(val);
        }
        else
        {
            fatalAssert(false, "Unsupported size for enum value");
        }
        enumVal = clearInvalidEnumValues(enumVal, enumProp);
        return enumVal;
    }
    FORCE_INLINE static void setValidEnumValue(void *val, SizeT enumVal, const EnumProperty *enumProp)
    {
        enumVal = clearInvalidEnumValues(enumVal, enumProp);
        if (enumProp->typeInfo->size == 1)
        {
            *reinterpret_cast<uint8 *>(val) = (uint8)(enumVal);
        }
        else if (enumProp->typeInfo->size == 2)
        {
            *reinterpret_cast<uint16 *>(val) = (uint16)(enumVal);
        }
        else if (enumProp->typeInfo->size == 4)
        {
            *reinterpret_cast<uint32 *>(val) = (uint32)(enumVal);
        }
        else if (enumProp->typeInfo->size == 8)
        {
            *reinterpret_cast<uint64 *>(val) = (uint64)(enumVal);
        }
        else
        {
            fatalAssert(false, "Unsupported size for enum value");
        }
    }

    template <ReflectClassOrStructType ChildType, ReflectClassOrStructType ParentType>
    FORCE_INLINE static bool isChildOf()
    {
        IReflectionRuntimeModule *rtti = IReflectionRuntimeModule::get();
        const ClassProperty *childClassProp = rtti->getClassType(typeInfoFrom<ChildType>());
        const ClassProperty *parentClassProp = rtti->getClassType(typeInfoFrom<ParentType>());
        return isChildOf(childClassProp, parentClassProp);
    }
    static bool isChildOf(const ClassProperty *childClassProp, const ClassProperty *parentClassProp);
    static bool isStruct(const ClassProperty *classProp);

    static const InterfaceInfo *getMatchingInterfaceInfo(const ClassProperty *childClassProp, const ReflectTypeInfo *interfaceType);
    template <ReflectClassType ChildType, InterfaceType ParentType>
    FORCE_INLINE static bool implementsInterface()
    {
        return implementsInterface(ChildType::staticType(), typeInfoFrom<ParentType>());
    }
    template <InterfaceType ParentType>
    FORCE_INLINE static bool implementsInterface(const ClassProperty *childClassProp)
    {
        return implementsInterface(childClassProp, typeInfoFrom<ParentType>());
    }
    static bool implementsInterface(const ClassProperty *childClassProp, const ReflectTypeInfo *interfaceType);

    template <typename ClassType, typename... CtorArgs>
    FORCE_INLINE static const GlobalFunctionWrapper *findMatchingCtor()
    {
        return findMatchingCtor<CtorArgs...>(IReflectionRuntimeModule::getClassType<ClassType>());
    }
    template <typename... CtorArgs>
    static const GlobalFunctionWrapper *findMatchingCtor(const ClassProperty *clazz)
    {
        for (const FunctionProperty *ctor : clazz->constructors)
        {
            if (ctor->funcPtr->isSameArgsType<CtorArgs...>())
            {
                return static_cast<const GlobalFunctionWrapper *>(ctor->funcPtr);
            }
        }
        return nullptr;
    }
    static const FieldProperty *findField(const ClassProperty *clazz, const StringID &fieldName)
    {
        auto itr = std::find_if(
            clazz->memberFields.cbegin(), clazz->memberFields.cend(),
            [&fieldName](const FieldProperty *prop) { return prop->name == fieldName; }
        );
        return (itr == clazz->memberFields.cend()) ? nullptr : *itr;
    }
    static const FieldProperty *findStaticField(const ClassProperty *clazz, const StringID &fieldName)
    {
        auto itr = std::find_if(
            clazz->staticFields.cbegin(), clazz->staticFields.cend(),
            [&fieldName](const FieldProperty *prop) { return prop->name == fieldName; }
        );
        return (itr == clazz->staticFields.cend()) ? nullptr : *itr;
    }
};