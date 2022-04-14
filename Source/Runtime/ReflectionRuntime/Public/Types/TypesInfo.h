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

#include "ReflectionRuntimeExports.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"
#include "Types/HashTypes.h"
#include "Types/Templates/TypeTraits.h"
#include "Types/Templates/ValueTraits.h"

#include <sstream>
#include <typeindex>
#include <typeinfo>

namespace EReflectTypeQualifiers
{
enum Type
{
    LReference = 1, // L-Val
    RReference = 2, // R-Val
    Pointer = 4,    // Pointer
    Constant = 8    // Const-ness of Pointer(Not pointed type) or reference(Not referenced type)
};
} // namespace EReflectTypeQualifiers
#define FOREACH_REFLECTTYPEQUALIFIER(MacroFn)                                                           \
    MacroFn(LReference) MacroFn(RReference) MacroFn(Pointer) MacroFn(Constant)

struct ReflectTypeInfo
{
    std::type_index typeID;
    const ReflectTypeInfo *innerType;
    SizeT size;       // sizeof(Type)
    uint32 alignment; // alignof(Type)
    uint32 qualifiers;

    std::strong_ordering operator<=>(const ReflectTypeInfo &otherTypeInfo) const = default;

    REFLECTIONRUNTIME_EXPORT static const ReflectTypeInfo *createTypeInfo(
        const std::type_info &cleanTypeInfo, const ReflectTypeInfo *innerTypeInfo, SizeT size,
        uint32 alignment, uint32 inQualifiers);
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
struct TypeSizeAndAlignment
{
    static SizeT sizeOf() { return sizeof(Type); }
    static uint32 alignOf() { return alignof(Type); }
};
template <typename T>
requires std::is_void_v<T>
struct TypeSizeAndAlignment<T>
{
    static SizeT sizeOf() { return 0; }
    static uint32 alignOf() { return 0; }
};
template <typename Type>
FORCE_INLINE const ReflectTypeInfo *typeInfoFrom()
{
    // Pointer can be const reference cannot be const
    using InnerType = std::conditional_t<std::is_pointer_v<Type>,
        std::remove_pointer_t<std::remove_const_t<Type>>, std::remove_reference_t<Type>>;

    static const ReflectTypeInfo *TYPE_INFO
        = ReflectTypeInfo::createTypeInfo(typeid(CleanType<Type>),
            (std::is_same_v<Type, InnerType> ? nullptr : typeInfoFrom<InnerType>()),
            TypeSizeAndAlignment<Type>::sizeOf(), TypeSizeAndAlignment<Type>::alignOf(),
            {
                ConditionalValue_v<uint32, uint32, std::is_lvalue_reference<Type>,
                    EReflectTypeQualifiers::LReference,
                    0> | ConditionalValue_v<uint32, uint32, std::is_rvalue_reference<Type>, EReflectTypeQualifiers::RReference, 0> | ConditionalValue_v<uint32, uint32, std::is_const<Type>, EReflectTypeQualifiers::Constant, 0> | ConditionalValue_v<uint32, uint32, std::is_pointer<Type>, EReflectTypeQualifiers::Pointer, 0>
                //| ConditionalValue_v<uint32, uint32, std::is_class<Type>,
                // EReflectTypeQualifiers::ClassType, 0> | ConditionalValue_v<uint32, uint32,
                // std::is_enum<Type>, EReflectTypeQualifiers::EnumType, 0>
            });

    return TYPE_INFO;
}

//
// auto testList = typeInfoListFrom<const int32&, const int32&&, const int32*, int32 const* const, int32
// const* const&, int32 const* const&&>();
//
template <typename... Types>
FORCE_INLINE std::vector<const ReflectTypeInfo *> typeInfoListFrom()
{
    std::vector<const ReflectTypeInfo *> retVal{ typeInfoFrom<Types>()... };
    return retVal;
}

#define REFLECTTYPEQUALIFIER_STR(QualifierEnum)                                                         \
    << (BIT_SET(ti.qualifiers, EReflectTypeQualifiers::##QualifierEnum##) ? TCHAR(" " #QualifierEnum)   \
                                                                          : TCHAR(""))
// Logger overrides
FORCE_INLINE OutputStream &operator<<(OutputStream &stream, const ReflectTypeInfo &ti)
{
    stream << TCHAR("Type info[0x") << std::hex << &ti << std::dec << TCHAR("]");
    stream << TCHAR("[Name:") << ti.typeID.name() << TCHAR(", ");
    stream << TCHAR("Hash : ") << ti.typeID.hash_code() << TCHAR(", ");
    stream << TCHAR("Size : ") << ti.size << TCHAR(", ");
    stream << TCHAR("Alignment : ") << ti.alignment << TCHAR(", ");
    stream << TCHAR("Qualifiers :(") FOREACH_REFLECTTYPEQUALIFIER(REFLECTTYPEQUALIFIER_STR)
           << TCHAR(" )");
    if (ti.innerType)
    {
        stream << TCHAR(", Inner type : ") << *ti.innerType;
    }
    stream << TCHAR("]");
    return stream;
}
#undef REFLECTTYPEQUALIFIER_STR

template <>
struct std::hash<ReflectTypeInfo>
{
    NODISCARD size_t operator()(const ReflectTypeInfo &keyval) const noexcept
    {
        size_t hashSeed = 0;
        // we do not have to hash size and alignment as they are just info variables
        HashUtility::hashAllInto(hashSeed, keyval.typeID, keyval.innerType, keyval.qualifiers);
        return hashSeed;
    }
};