/*!
 * \file FieldVisitors.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypes.h"
#include "Property/CustomProperty.h"
#include "Property/Property.h"
#include "ReflectionMacros.h"
#include "ReflectionRuntimeExports.h"
#include "Types/Colors.h"
#include "Types/PropertyTypes.h"
#include "Types/Templates/TypeList.h"
#include "Types/Transform3D.h"

#define IS_TYPE_SAME_FIRST(TypeName) std::is_same<Type, TypeName>
#define IS_TYPE_SAME(TypeName) , std::is_same<Type, TypeName>

template <typename Type>
concept IsReflectedFundamental_Internal = std::disjunction_v<FOR_EACH_CORE_TYPES_UNIQUE_FIRST_LAST(
    IS_TYPE_SAME_FIRST, IS_TYPE_SAME, IS_TYPE_SAME)>;
template <typename Type>
concept IsReflectedSpecial_Internal = std::disjunction_v<FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(
    IS_TYPE_SAME_FIRST, IS_TYPE_SAME, IS_TYPE_SAME)>;

template <typename Type>
concept IsReflectedFundamental = IsReflectedFundamental_Internal<UnderlyingType<Type>>;
template <typename Type>
concept IsReflectedSpecial = IsReflectedSpecial_Internal<UnderlyingType<Type>>;

#undef IS_TYPE_SAME
#undef IS_TYPE_SAME_FIRST

struct PropertyInfo
{
    const ClassProperty *rootProperty = nullptr;
    const FieldProperty *fieldProperty = nullptr;
    const TypedProperty *parentProperty = nullptr;
    const TypedProperty *thisProperty = nullptr;
};

class REFLECTIONRUNTIME_EXPORT FieldVisitor
{
private:
    FieldVisitor() = default;

    /**
     * Must have visit function that has following signature
     * typename Visitable - Must be a lambda taking template type as operator call
     *
     */
    template <typename Type, typename Visitable>
    static void qualifyField(bool bIsPtr, bool bIsConstPtr, PropertyInfo propInfo, Visitable &&visitor)
    {
        if (bIsPtr)
        {
            if (bIsConstPtr)
            {
                visitor.operator()<Type const *>(propInfo);
            }
            else
            {
                visitor.operator()<Type *>(propInfo);
            }
        }
        else
        {
            fatalAssert(false, "References in field is not allowed");
        }
    }
    template <typename Visitable>
    static void visitQualifiedField(PropertyInfo propInfo, Visitable &&visitor)
    {
        // Qualified type cannot have more than 1 inner type as that means double or more
        // reference/pointer combination
        fatalAssert(propInfo.thisProperty->typeInfo->innerType == nullptr
                        || propInfo.thisProperty->typeInfo->innerType->innerType == nullptr,
            "Qualification for property %s is not allowed for field types in field %s",
            propInfo.thisProperty->nameString, propInfo.fieldProperty->nameString);

        bool bIsInnerConst = propInfo.thisProperty->typeInfo->innerType
                             && BIT_SET(propInfo.thisProperty->typeInfo->innerType->qualifiers,
                                 EReflectTypeQualifiers::Constant);
        bool bIsPtr
            = BIT_SET(propInfo.thisProperty->typeInfo->qualifiers, EReflectTypeQualifiers::Pointer);
        bool bIsConstPtr
            = BIT_SET(propInfo.thisProperty->typeInfo->qualifiers, EReflectTypeQualifiers::Constant);
        const TypedProperty *parentProp = propInfo.parentProperty;
        auto typeQualifierVisitor = [bIsInnerConst, bIsPtr, bIsConstPtr, parentProp,
                                        &visitor]<typename Type>(PropertyInfo propInfo)
        {
            propInfo.thisProperty = propInfo.parentProperty;
            propInfo.parentProperty = parentProp;
            if (bIsInnerConst)
            {
                qualifyField<const Type>(bIsPtr, bIsConstPtr, propInfo, std::move(visitor));
            }
            else
            {
                qualifyField<Type>(bIsPtr, bIsConstPtr, propInfo, std::move(visitor));
            }
        };
        propInfo.parentProperty = propInfo.thisProperty;
        propInfo.thisProperty = static_cast<const TypedProperty *>(
            static_cast<const QualifiedProperty *>(propInfo.parentProperty)->unqualTypeProperty);
        visit<std::remove_cvref_t<decltype(typeQualifierVisitor)>, EPropertyType::QualifiedType>(
            propInfo, std::move(typeQualifierVisitor));
    }
#define CHECK_FIELD_TYPE(CheckTypeName)                                                                 \
    if (propInfo.thisProperty->typeInfo == typeInfoFrom<CheckTypeName>())                               \
    {                                                                                                   \
        visitor.operator()<CheckTypeName>(propInfo);                                                    \
        return;                                                                                         \
    }

    template <typename Visitable>
    static void visitFundamentalType(PropertyInfo propInfo, Visitable &&visitor)
    {
        FOR_EACH_CORE_TYPES(CHECK_FIELD_TYPE);
    }

    template <typename Visitable>
    static void visitSpecialType(PropertyInfo propInfo, Visitable &&visitor)
    {
        FOR_EACH_SPECIAL_TYPES(CHECK_FIELD_TYPE);
    }

#undef CHECK_FIELD_TYPE

    template <typename Visitable, EPropertyType... IgnoredTypes>
    static void visit(PropertyInfo propInfo, Visitable &&visitor)
    {
        using IgnoredProps = TL::CreateFromUInts<(uint64)(IgnoredTypes)...>::type;
        if (propInfo.thisProperty == nullptr)
            propInfo.thisProperty = static_cast<const TypedProperty *>(propInfo.fieldProperty->field);

        switch (propInfo.thisProperty->type)
        {
        case EPropertyType::QualifiedType:
            if CONST_EXPR (!TL::Contains<IgnoredProps,
                               UIntToType<(uint64)(EPropertyType::QualifiedType)>>::value)
            {
                visitQualifiedField(propInfo, std::forward<Visitable>(visitor));
            }
            else
            {
                alertIf(false, "Qualified type invoked inside qualified type, Use struct");
            }
            break;
        case EPropertyType::FundamentalType:
            visitFundamentalType(propInfo, std::forward<Visitable>(visitor));
            break;
        case EPropertyType::SpecialType:
            visitSpecialType(propInfo, std::forward<Visitable>(visitor));
            break;
        // It is best to allow this be handled by caller and only handle basic defined data types
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        case EPropertyType::ClassType:
        case EPropertyType::EnumType:
        {
            visitor.operator()<void>(propInfo);
            break;
        }
        default:
            break;
        }
    }

public:
    /**
     *
     * typename Visitable;
     * Must have visit function that has following signature
     *      template <typename Type>
     *      static void visit(Type* val, const PropertyInfo& propInfo, void* userData);
     *
     * In case of custom types like map or vector or pair the values if nested into another custom type
     * has to be handled manually by caller when receiving call back with types
     */
    template <typename Visitable>
    static void visitStaticFields(const ClassProperty *root, void *userData)
    {
        for (const ClassProperty *baseClass : root->baseClasses)
        {
            visitStaticFields<Visitable>(baseClass, userData);
        }
        auto staticVisitor = [userData]<typename Type>(PropertyInfo propInfo)
        {
            // Since getAsType needs valid type we cannot use it in void cases
            FieldValuePtr<Type> value;
            if CONST_EXPR (std::is_void_v<Type>)
            {
                value = static_cast<const GlobalFieldWrapper *>(propInfo.fieldProperty->fieldPtr)->get();
            }
            else
            {
                value = static_cast<const GlobalFieldWrapper *>(propInfo.fieldProperty->fieldPtr)
                            ->getAsTypeUnsafe<Type>();
            }

            if (BIT_SET(propInfo.thisProperty->typeInfo->qualifiers, EReflectTypeQualifiers::Constant))
            {
                Visitable::visit(value.constVPtr, propInfo, userData);
            }
            else
            {
                Visitable::visit(value.vPtr, propInfo, userData);
            }
        };

        PropertyInfo propInfo{ .rootProperty = root };
        for (const FieldProperty *field : root->staticFields)
        {
            propInfo.fieldProperty = field;
            visit(propInfo, staticVisitor);
        }
    }
    template <typename Visitable>
    static void visitFields(const ClassProperty *root, void *rootObject, void *userData)
    {
        for (const ClassProperty *baseClass : root->baseClasses)
        {
            visitFields<Visitable>(baseClass, rootObject, userData);
        }

        auto memberVisitor = [userData, rootObject]<typename Type>(PropertyInfo propInfo)
        {
            void *value = static_cast<const MemberFieldWrapper *>(propInfo.fieldProperty->fieldPtr)
                              ->get(rootObject);
            Visitable::visit(reinterpret_cast<Type *>(value), propInfo, userData);
        };

        PropertyInfo propInfo{ .rootProperty = root };
        for (const FieldProperty *field : root->memberFields)
        {
            propInfo.fieldProperty = field;
            visit(propInfo, memberVisitor);
        }
    }
    template <typename Visitable>
    static void visitFields(const ClassProperty *root, const void *rootObject, void *userData)
    {
        for (const ClassProperty *baseClass : root->baseClasses)
        {
            visitFields<Visitable>(baseClass, rootObject, userData);
        }

        auto memberVisitor = [userData, rootObject]<typename Type>(PropertyInfo propInfo)
        {
            const void *value = static_cast<const MemberFieldWrapper *>(propInfo.fieldProperty->fieldPtr)
                                    ->get(rootObject);
            Visitable::visit(reinterpret_cast<const Type *>(value), propInfo, userData);
        };

        PropertyInfo propInfo{ .rootProperty = root };
        for (const FieldProperty *field : root->memberFields)
        {
            propInfo.fieldProperty = field;
            visit(propInfo, memberVisitor);
        }
    }

    // Just simple visits a visitable with given template typename and passes in userdata
    template <typename Visitable>
    static void visit(const PropertyInfo &propInfo, void *userData)
    {
        auto typeVisitor = [userData]<typename Type>(PropertyInfo propInfo)
        { Visitable::template visit<Type>(propInfo, userData); };
        visit(propInfo, typeVisitor);
    }

    // Visitor to visit a typed visitable for given property and passes in value as type ptr
    template <typename Visitable>
    static void visit(const TypedProperty *prop, void *val, void *userData)
    {
        auto typeVisitor = [val, userData]<typename Type>(PropertyInfo propInfo)
        { Visitable::visit(reinterpret_cast<Type *>(val), propInfo, userData); };
        PropertyInfo propInfo;
        propInfo.thisProperty = prop;
        visit(propInfo, typeVisitor);
    }
};