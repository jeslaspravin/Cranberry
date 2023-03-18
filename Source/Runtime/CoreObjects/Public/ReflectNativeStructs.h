/*!
 * \file ReflectNativeStructs.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#ifndef __REF_PARSE__

#ifndef REFLECTNATIVESTRUCTS_GEN_TU
#error "ReflectNativeStruct.h must not be included from any where"
#endif // REFLECTNATIVESTRUCTS_GEN_TU

// It is okay, This file must not be included from anywhere else except ReflectNativeStructs.gen.cpp in runtime
#include "ObjectPtrs.h"

#else

#include "ReflectionMacros.h"
#include "CBEObjectTypes.h"
#include "ReflectNativeStructs.gen.h"

namespace cbe
{

struct META_ANNOTATE(DisableCtor) ObjectPath
{
    GENERATED_CODES()

    META_ANNOTATE()
    ObjectAllocIdx allocIdx;

    META_ANNOTATE()
    String packagePath;

    META_ANNOTATE()
    String outerPath;

    META_ANNOTATE()
    String objectName;
};

} // namespace cbe

#endif // __REF_PARSE__
