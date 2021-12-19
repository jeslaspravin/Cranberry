#include "Property/CustomProperty.h"

CustomProperty::CustomProperty(const String& propName, EPropertyType propType, const ReflectTypeInfo* propTypeInfo)
    : TypedProperty(propName, propType, propTypeInfo)
{}

CustomProperty::~CustomProperty()
{
    delete dataRetriever;
    dataRetriever = nullptr;
}
