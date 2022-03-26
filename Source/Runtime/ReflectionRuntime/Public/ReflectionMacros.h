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

#define FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(String) \
    MacroName(StringID) \
    MacroName(Color) \
    MacroName(LinearColor) \
    MacroName(Vector2D) \
    MacroName(Vector3D) \
    MacroName(Vector4D) \
    MacroName(Matrix2) \
    MacroName(Matrix3) \
    MacroName(Matrix4) \
    MacroName(Rotation) \
    LastMacroName(Transform3D)

#define FOR_EACH_SPECIAL_TYPES(MacroName) FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

#define FOR_EACH_CUSTOM_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(std::pair) \
    MacroName(std::vector) \
    MacroName(std::map) \
    MacroName(std::unordered_map) \
    MacroName(std::set) \
    LastMacroName(std::unordered_set)

#define FOR_EACH_CUSTOM_TYPES(MacroName) FOR_EACH_CUSTOM_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

#define COMBINE_GENERATED_CODES_internal(A,B,C) A##_##B##C
#define COMBINE_GENERATED_CODES(A,B,C) COMBINE_GENERATED_CODES_internal(A, B, C)
#define GENERATED_CODES_ALIAS GeneratedCodeLine
// If present in base class with Build Flag BaseType then generator will not setup DefaultConstructionPolicy in Base class
#define OVERRIDEN_CONSTRUCTION_POLICY_ALIAS OverridenCtorPolicy
#define CONSTRUCTION_POLICY_TYPEDEF_NAME HeapConstructionPolicy
// If parsing reflection we do not have definition for below macro so we ignore it
#ifdef __REF_PARSE__
// To allow clang parser to detect generated codes
#define GENERATED_CODES() using GENERATED_CODES_ALIAS = uint32;
#define OVERRIDE_CONSTRUCTION_POLICY(PolicyTypeName) using OVERRIDEN_CONSTRUCTION_POLICY_ALIAS = uint32;

#define META_ANNOTATE(...) __attribute__((annotate( #__VA_ARGS__ )))
#define META_ANNOTATE_API(API_EXPORT, ...) META_ANNOTATE(NoExport;__VA_ARGS__)
#else
#define GENERATED_CODES() COMBINE_GENERATED_CODES(HEADER_FILE_ID, __LINE__, _GENERATED_CODES)
#define OVERRIDE_CONSTRUCTION_POLICY(PolicyTypeName) typedef PolicyTypeName CONSTRUCTION_POLICY_TYPEDEF_NAME;
#define META_ANNOTATE(...)
#define META_ANNOTATE_API(API_EXPORT, ...) API_EXPORT
#endif