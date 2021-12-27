#pragma once
#include "Types/CoreTypes.h"
#include "ReflectionRuntimeExports.h"

#include <utility>

struct ReflectTypeInfo;

/*
* Meta flags for struct and class types
*/
enum ClassMetaFlags : uint64
{
    /* Any class that is going to be base reflect class for a hierarchy of reflected classes */
    CLASSMETA_BaseType = 1
};

/*
* Meta flags for enum and its fields
*/
enum EnumMetaFlags : uint64
{
};

/*
* Meta flags for fields
*/
enum FieldMetaFlags : uint64
{
};

/*
* Meta flags for functions
*/
enum FunctionMetaFlags : uint64
{
};

class REFLECTIONRUNTIME_EXPORT PropertyMetaDataBase
{
public:
    virtual ~PropertyMetaDataBase() = default;
    virtual const ReflectTypeInfo* metaType() const = 0;
};