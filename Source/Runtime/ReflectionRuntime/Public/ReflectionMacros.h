/*!
 * \file ReflectionMacros.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Property/Property.h"

// clang-format off
#define FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(String)          \
    MacroName(StringID)             \
    MacroName(NameString)           \
    MacroName(Color)                \
    MacroName(LinearColor)          \
    MacroName(Vector2D)             \
    MacroName(Vector3D)             \
    MacroName(Vector4D)             \
    MacroName(Matrix2)              \
    MacroName(Matrix3)              \
    MacroName(Matrix4)              \
    MacroName(Rotation)             \
    MacroName(Quat)                 \
    LastMacroName(Transform3D)

#define FOR_EACH_SPECIAL_TYPES(MacroName) FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

#define FOR_EACH_MAP_SET_TYPES(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(std::map)            \
    MacroName(std::unordered_map)       \
    MacroName(std::set)                 \
    LastMacroName(std::unordered_set)

#define FOR_EACH_CUSTOM_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)   \
    FirstMacroName(std::pair)                                                               \
    MacroName(std::vector)                                                                  \
    FOR_EACH_MAP_SET_TYPES(MacroName, MacroName, LastMacroName)

#define FOR_EACH_CUSTOM_TYPES(MacroName) FOR_EACH_CUSTOM_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

#define CONTAINERTYPE_VALIDATIONS(TypeName, ValueTypeName)                                                  \
    static_assert(                                                                                          \
        std::is_same_v<UnderlyingType<TypeName>, std::vector<ValueTypeName>>                                \
        || std::is_same_v<UnderlyingType<TypeName>, std::set<ValueTypeName>>                                \
        || std::is_same_v<UnderlyingType<TypeName>, std::unordered_set<ValueTypeName>>,                     \
        "Only basic vector, set and unordered set with default allocator and hash functions are supported!" \
)

#define MAPTYPE_VALIDATIONS(TypeName, KeyTypeName, ValueTypeName)                                     \
    static_assert(                                                                                    \
        std::is_same_v<UnderlyingType<TypeName>, std::map<KeyTypeName, ValueTypeName>>                \
        || std::is_same_v<UnderlyingType<TypeName>, std::unordered_map<KeyTypeName, ValueTypeName>>,  \
        "Only basic map and unordered map with default allocator and hash functions are supported!"   \
    )

// clang-format on

#define COMBINE_GENERATED_CODES_internal(A, B, C) A##_##B##C
#define COMBINE_GENERATED_CODES(A, B, C) COMBINE_GENERATED_CODES_internal(A, B, C)
#define GENERATED_CODES_ALIAS GeneratedCodeLine
#define GENERATED_INTERFACE_CODES_ALIAS GeneratedInterfaceCodeLine

#define GENERATED_INTERFACE_CODES()                                                                                                            \
public:                                                                                                                                        \
    using GENERATED_INTERFACE_CODES_ALIAS = uint32;                                                                                            \
    virtual const ClassProperty *getType() const = 0;

// If present in base class with Build Flag BaseType then generator will not setup
// DefaultConstructionPolicy in Base class
#define OVERRIDEN_CONSTRUCTION_POLICY_ALIAS OverridenCtorPolicy
#define CONSTRUCTION_POLICY_TYPEDEF_NAME HeapConstructionPolicy
// If parsing reflection we do not have definition for below macro so we ignore it
#ifdef __REF_PARSE__
// To allow clang parser to detect generated codes
#define GENERATED_CODES()                                                                                                                      \
    using GENERATED_CODES_ALIAS = uint32;                                                                                                      \
    COMPILER_PRAGMA(COMPILER_PUSH_WARNING)                                                                                                     \
    COMPILER_PRAGMA(COMPILER_DISABLE_WARNING(WARN_MISSING_OVERRIDE))                                                                           \
    /* To not consider Generated class with interface as abstract */                                                                           \
    const ClassProperty *getType() const { return nullptr; }                                                                                   \
    COMPILER_PRAGMA(COMPILER_POP_WARNING)

#define OVERRIDE_CONSTRUCTION_POLICY(PolicyTypeName) using OVERRIDEN_CONSTRUCTION_POLICY_ALIAS = uint32;

#define META_ANNOTATE(...) __attribute__((annotate(#__VA_ARGS__)))
#define META_ANNOTATE_API(API_EXPORT, ...) META_ANNOTATE(NoExport; __VA_ARGS__)
#else
#define GENERATED_CODES() COMBINE_GENERATED_CODES(HEADER_FILE_ID, __LINE__, _GENERATED_CODES)
#define OVERRIDE_CONSTRUCTION_POLICY(PolicyTypeName) using CONSTRUCTION_POLICY_TYPEDEF_NAME = PolicyTypeName;
#define META_ANNOTATE(...)
#define META_ANNOTATE_API(API_EXPORT, ...) API_EXPORT

#endif