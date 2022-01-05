/*!
 * \file CustomProperty.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Property/CustomProperty.h"

CustomProperty::CustomProperty(const String& propName, EPropertyType propType, const ReflectTypeInfo* propTypeInfo)
    : TypedProperty(propName, propType, propTypeInfo)
{}

CustomProperty::~CustomProperty()
{
    delete dataRetriever;
    dataRetriever = nullptr;
}
