/*!
 * \file ReflectionCommonHeaders.h
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

#include "Types/DefaultPolicies.h"
#include "Types/TypesInfo.h"

#include "Types/FunctionTypes.h"
#include "Types/PropertyTypes.h"

#include "Property/PropertyMetaData.h"
#include "Property/Property.h"

#include "IReflectionRuntime.h"
#include "ReflectionMacros.h"

// Since new is overridden for each class and struct
#include "Memory/Memory.h"