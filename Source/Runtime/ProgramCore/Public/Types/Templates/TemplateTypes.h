/*!
 * \file TemplateTypes.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"

// Just some common types
struct NullType
{};

struct EmptyType
{};

// Types to differentiate function overload signatures
template <int IntValue>
struct IntToType
{
    CONST_EXPR static const int value = IntValue;
};

template <typename Type>
struct TypeToType
{
    using type = Type;
};