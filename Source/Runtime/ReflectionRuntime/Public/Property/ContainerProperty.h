/*!
 * \file ContainerProperty.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Property/CustomProperty.h"

#include <set>
#include <unordered_set>


template <typename ContainerType>
struct ContainerEPropertyType
{};

// Array type
template <typename... Args>
struct ContainerEPropertyType<std::vector<Args...>>
{
    static CONST_EXPR EPropertyType type = EPropertyType::ArrayType;
};
template <typename... Args>
struct ContainerEPropertyType<std::array<Args...>>
{
    static CONST_EXPR EPropertyType type = EPropertyType::ArrayType;
};

template <typename... Args>
struct ContainerEPropertyType<std::set<Args...>>
{
    static CONST_EXPR EPropertyType type = EPropertyType::SetType;
};
template <typename... Args>
struct ContainerEPropertyType<std::unordered_set<Args...>>
{
    static CONST_EXPR EPropertyType type = EPropertyType::SetType;
};

template <typename ContainerType>
class ContainerPropertyImpl final : public ContainerProperty
{
public:
    const BaseProperty * elementProp;
public:
    ContainerPropertyImpl(const String& propName, const ReflectTypeInfo* propTypeInfo)
        : ContainerProperty(propName, ContainerEPropertyType<ContainerType>::type, propTypeInfo)
    {}

    FORCE_INLINE ContainerPropertyImpl* setElementProperty(const BaseProperty* elementProperty)
    {
        elementProp = elementProperty;
        return this;
    }
};