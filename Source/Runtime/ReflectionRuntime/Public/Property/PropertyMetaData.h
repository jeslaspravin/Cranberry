/*!
 * \file PropertyMetaData.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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
 * Meta flags for struct and class types, Flags must be shifted using INDEX_TO_FLAG_MASK() before testing against uint64
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
 * Meta flags for enum and its fields, Flags must be shifted using INDEX_TO_FLAG_MASK() before testing against uint64
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
 * Meta flags for fields, Flags must be shifted using INDEX_TO_FLAG_MASK() before testing against uint64
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
 * Meta flags for functions, Flags must be shifted using INDEX_TO_FLAG_MASK() before testing against uint64
 */
enum EFunctionMetaFlags : uint64
{
    FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
};
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY

/**
 * Must be simple pod type without any custom constructor.
 * However if you intend to add custom constructor
 * Be sure to have "const ReflectTypeInfo *type" as first parameter and pass it along to PropertyMetaDataBase{ type }
 *
 * Example :
 * struct SecondTest : public PropertyMetaDataBase
 * {
 *     int32 idx;
 *     String str;
 *
 *     SecondTest(const ReflectTypeInfo *type, int32 i)
 *         : PropertyMetaDataBase{ type }
 *         , idx(i)
 *     {}
 *     SecondTest(const ReflectTypeInfo *type, String s)
 *         : PropertyMetaDataBase{ type }
 *         , str(s)
 *     {}
 * };
 *
 */
struct PropertyMetaDataBase
{
public:
    const ReflectTypeInfo *metaType = nullptr;
};