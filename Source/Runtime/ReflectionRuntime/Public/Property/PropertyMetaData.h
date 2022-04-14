/*!
 * \file PropertyMetaData.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Property/PropertyMetaFlags.inl"
#include "ReflectionRuntimeExports.h"
#include "Types/CoreTypes.h"

#include <utility>

struct ReflectTypeInfo;

#define META_FLAG_ENTRY_FIRST(Flag) CLASSMETA_##Flag
#define META_FLAG_ENTRY(Flag) , CLASSMETA_##Flag
/*
 * Meta flags for struct and class types
 */
enum EClassMetaFlags : uint64
{
    FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
};
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY

#define META_FLAG_ENTRY_FIRST(Flag) ENUMMETA_##Flag
#define META_FLAG_ENTRY(Flag) , ENUMMETA_##Flag
/*
 * Meta flags for enum and its fields
 */
enum EEnumMetaFlags : uint64
{
    FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
};
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY

#define META_FLAG_ENTRY_FIRST(Flag) FIELDMETA_##Flag
#define META_FLAG_ENTRY(Flag) , FIELDMETA_##Flag
/*
 * Meta flags for fields
 */
enum EFieldMetaFlags : uint64
{
    FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
};
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY

#define META_FLAG_ENTRY_FIRST(Flag) FUNCMETA_##Flag
#define META_FLAG_ENTRY(Flag) , FUNCMETA_##Flag
/*
 * Meta flags for functions
 */
enum EFunctionMetaFlags : uint64
{
    FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
};
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY

class REFLECTIONRUNTIME_EXPORT PropertyMetaDataBase
{
public:
    virtual ~PropertyMetaDataBase() = default;
    virtual const ReflectTypeInfo *metaType() const = 0;
};