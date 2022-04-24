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
};