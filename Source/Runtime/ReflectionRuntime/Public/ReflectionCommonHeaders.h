/*!
 * \file ReflectionCommonHeaders.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ReflectionRuntimeExports.h"

// Since pointer has transparent hasher and comparer
#include "Types/HashTypes.h"
// Since new is overridden for each class and struct
#include "Memory/Memory.h"

#include "Types/DefaultPolicies.h"
#include "Types/TypesInfo.h"

#include "Types/FunctionTypes.h"
#include "Types/PropertyTypes.h"

#include "Property/Property.h"
#include "Property/PropertyMetaData.h"

#include "IReflectionRuntime.h"
#include "ReflectionMacros.h"
