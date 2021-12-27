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

#define COMBINE_GENERATED_CODES(A,B,C,D) A##B##C##D
// If parsing reflection we do not have definition for below macro so we ignore it
#ifdef __REF_PARSE__
#define GENERATED_CODES()
#define META_ANNOTATE(API_EXPORT, ...) __attribute__((annotate( #__VA_ARGS__ )))
#define META_ANNOTATE_API(API_EXPORT, ...) META_ANNOTATE(__VA_ARGS__)
#else
#define GENERATED_CODES() COMBINE_GENERATED_CODES(HEADER_FILE_ID,_,__LINE__,_GENERATED_CODES)
#define META_ANNOTATE(...)
#define META_ANNOTATE_API(API_EXPORT, ...) API_EXPORT
#endif