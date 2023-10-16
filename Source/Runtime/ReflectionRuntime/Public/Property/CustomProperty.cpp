/*!
 * \file CustomProperty.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Property/CustomProperty.h"

CustomProperty::CustomProperty(const StringID &propNameID, const TChar *propName, EPropertyType propType, const ReflectTypeInfo *propTypeInfo)
    : TypedProperty(propNameID, propName, propType, propTypeInfo)
    , dataRetriever(nullptr)
{}

CustomProperty::~CustomProperty()
{
    delete dataRetriever;
    dataRetriever = nullptr;
}
