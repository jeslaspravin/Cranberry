/*!
 * \file TypesInfo.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Traits/TypeTraits.h"
#include "Types/Traits/ValueTraits.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"
#include "Types/HashTypes.h"
#include "ReflectionRuntimeExports.h"

#include <typeinfo>
#include <typeindex>
#include <sstream>

namespace EReflectTypeQualifiers
{
    enum Type
    {
        LReference = 1, // L-Val
        RReference = 2, // R-Val
        Pointer = 4, // Pointer
        Constant = 8 // Const-ness of Pointer(Not pointed type) or reference(Not referenced type)
    };
}
#define FOREACH_REFLECTTYPEQUALIFIER(MacroFn) \
    MacroFn(LReference) \
    MacroFn(RReference) \
    MacroFn(Pointer)    \
    MacroFn(Constant)

struct ReflectTypeInfo
{
    std::type_index typeID;
    const ReflectTypeInfo* innerType;
    uint32 qualifiers;

    std::strong_ordering operator<=>(const ReflectTypeInfo& otherTypeInfo) const = default;

    REFLECTIONRUNTIME_EXPORT static const ReflectTypeInfo* createTypeInfo(const std::type_info& cleanTypeInfo, const ReflectTypeInfo* innerTypeInfo, uint32 inQualifiers);
};

template <typename Type>
using CleanType = UnderlyingType<Type>;
//
//  int a = 0;
//  int const* const aPtr = &a;// Here aPtr is const pointer pointing to const type
//  int const* const aPtr2 = &a;// Here aPtr2 is const pointer pointing to const type 
//  int const* const& aPtrRef = aPtr; // Here aPtrRef is const not the referencing ptr
//  aPtrRef = aPtr2;
//  aPtr = aPtr2;
// 
//  Test codes below
// 
//  LOG("Test", "Test type info \n%s\n%s\n%s\n%s\n%s\n%s"
//      // Referenced variable is const
//      , *typeInfoFrom<const int32&>()
//      , *typeInfoFrom<const int32&&>()
//      // Pointer to const variable
//      , *typeInfoFrom<const int32*>()
//      // Const pointer to const variable
//      , *typeInfoFrom<int32 const* const>()
//      // reference to Const pointer to const variable
//      , *typeInfoFrom<int32 const* const&>()
//      , *typeInfoFrom<int32 const* const&&>()
//  );
//  LOG("Test", "Test type info %d, %d, %d"
//      , typeInfoFrom<const int32&>() == typeInfoFrom<const int32&&>()
//      , typeInfoFrom<const int32*>() == typeInfoFrom<int32 const* const&>()
//      , typeInfoFrom<int32 const* const&>() == typeInfoFrom<int32 const* const&&>()
//  );
// 
template <typename Type>
FORCE_INLINE const ReflectTypeInfo* typeInfoFrom()
{
    // Pointer can be const reference cannot be const
    using InnerType = std::conditional_t<std::is_pointer_v<Type>
        , std::remove_pointer_t<std::remove_const_t<Type>>
        , std::remove_reference_t<Type>>;

    static const ReflectTypeInfo* TYPE_INFO = ReflectTypeInfo::createTypeInfo
    (
        typeid(CleanType<Type>),
        (std::is_same_v<Type, InnerType>? nullptr : typeInfoFrom<InnerType>()),
        {
            ConditionalValue_v<uint32, uint32, std::is_lvalue_reference<Type>, EReflectTypeQualifiers::LReference, 0>
            | ConditionalValue_v<uint32, uint32, std::is_rvalue_reference<Type>, EReflectTypeQualifiers::RReference, 0>
            | ConditionalValue_v<uint32, uint32, std::is_const<Type>, EReflectTypeQualifiers::Constant, 0>
            | ConditionalValue_v<uint32, uint32, std::is_pointer<Type>, EReflectTypeQualifiers::Pointer, 0>
            //| ConditionalValue_v<uint32, uint32, std::is_class<Type>, EReflectTypeQualifiers::ClassType, 0>
            //| ConditionalValue_v<uint32, uint32, std::is_enum<Type>, EReflectTypeQualifiers::EnumType, 0>
        }
    );

    return TYPE_INFO;
}

//
// auto testList = typeInfoListFrom<const int32&, const int32&&, const int32*, int32 const* const, int32 const* const&, int32 const* const&&>();
//
template <typename... Types>
FORCE_INLINE std::vector<const ReflectTypeInfo*> typeInfoListFrom()
{
    std::vector<const ReflectTypeInfo*> retVal{ typeInfoFrom<Types>()... };
    return retVal;
}

#define REFLECTTYPEQUALIFIER_STR(QualifierEnum) \
    << (BIT_SET(str.qualifiers, EReflectTypeQualifiers::##QualifierEnum##) ? TCHAR(" "#QualifierEnum) : TCHAR(""))
// Logger overrides
FORCE_INLINE OutputStream& operator<<(OutputStream& stream, const ReflectTypeInfo& str)
{
    stream << TCHAR("Type info[0x") << std::hex << &str << std::dec << TCHAR("]");
    stream << TCHAR("[Name:") << str.typeID.name() << TCHAR(", ");
    stream << TCHAR("Hash : ") << str.typeID.hash_code() << TCHAR(", ");
    stream << TCHAR("Qualifiers :(")
        FOREACH_REFLECTTYPEQUALIFIER(REFLECTTYPEQUALIFIER_STR)
        << TCHAR(" )");
    if(str.innerType)
    {
        stream << TCHAR(", Inner type : ") << *str.innerType;
    }
    stream << TCHAR("]");
    return stream;
}
#undef REFLECTTYPEQUALIFIER_STR

template <>
struct std::hash<ReflectTypeInfo>
{
    NODISCARD size_t operator()(const ReflectTypeInfo& keyval) const noexcept 
    {
        size_t hashSeed = 0;
        HashUtility::hashAllInto(hashSeed, keyval.typeID, keyval.innerType, keyval.qualifiers);
        return hashSeed;
    }
};