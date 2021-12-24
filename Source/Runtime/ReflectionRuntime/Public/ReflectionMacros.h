#pragma once

#include "Property/Property.h"

template <typename Type, StringLiteral TypeName>
BaseProperty* createQualifiedProperty()
{
    return new QualifiedProperty(TypeName.value, typeInfoFrom<Type>());
}
template <typename Type>
void initQualifiedProperty(BaseProperty* prop)
{
    QualifiedProperty* p = static_cast<QualifiedProperty*>(prop);
    p->setUnqualifiedType(IReflectionRuntimeModule::getType<CleanType<Type>>());
}

#define CREATE_QUALIFIED_PROPERTY(TypeName) IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createQualifiedProperty<TypeName, #TypeName>, &initQualifiedProperty<TypeName> });

#define FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(String) \
    MacroName(Color) \
    MacroName(LinearColor) \
    MacroName(Vector2D) \
    MacroName(Vector3D) \
    MacroName(Vector4D) \
    MacroName(Matrix2) \
    MacroName(Matrix3) \
    MacroName(Matrix4) \
    MacroName(Rotation) \
    LastMacroName(Transform3D)

#define FOR_EACH_SPECIAL_TYPES(MacroName) FOR_EACH_SPECIAL_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)