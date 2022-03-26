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
template <size_t IntValue>
struct IntToType
{
    CONST_EXPR static const size_t value = IntValue;
};

template <typename Type>
struct TypeToType
{
    using type = Type;
};