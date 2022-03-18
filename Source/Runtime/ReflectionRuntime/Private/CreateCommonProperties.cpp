/*!
 * \file CreateCommonProperties.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ReflectionRuntimeModule.h"
#include "ReflectionMacros.h"
#include "Types/TypesInfo.h"
#include "Math/CoreMathTypes.h"
#include "Types/Colors.h"
#include "Types/Transform3D.h"
#include "String/TCharString.h"

template <typename Type, StringLiteral TypeName>
BaseProperty* createFundamentalProperty()
{
    return new TypedProperty(TypeName.value, EPropertyType::FundamentalType, typeInfoFrom<Type>());
}

template <typename Type, StringLiteral TypeName>
BaseProperty* createSpecialProperty()
{
    return new TypedProperty(TypeName.value, EPropertyType::SpecialType, typeInfoFrom<Type>());
}

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

#define CREATE_QUALIFIED_PROPERTIES(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName&>(), { &createQualifiedProperty<TypeName&, TCHAR(#TypeName" &")>, &initQualifiedProperty<TypeName&> }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName*>(), { &createQualifiedProperty<TypeName*, TCHAR(#TypeName" *")>, &initQualifiedProperty<TypeName*> }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName*>(), { &createQualifiedProperty<const TypeName*, TCHAR("const "#TypeName" *")>, &initQualifiedProperty<TypeName*> }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName&>(), { &createQualifiedProperty<const TypeName&, TCHAR("const "#TypeName" &")>, &initQualifiedProperty<TypeName*> });

#define CREATE_FUNDAMENTAL_PROPERTY(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createFundamentalProperty<TypeName, TCHAR(#TypeName)>, nullptr }); \
    CREATE_QUALIFIED_PROPERTIES(TypeName)

#define CREATE_SPECIAL_PROPERTY(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createSpecialProperty<TypeName, TCHAR(#TypeName)>, nullptr }); \
    CREATE_QUALIFIED_PROPERTIES(TypeName)

void ReflectionRuntimeModule::initCommonProperties() const
{
    FOR_EACH_CORE_TYPES(CREATE_FUNDAMENTAL_PROPERTY);
    // Register void as well
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<void>(), { &createFundamentalProperty<void, TCHAR("void")>, nullptr });
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<void*>(), { &createQualifiedProperty<void*, TCHAR("void*")>, &initQualifiedProperty<void*> });
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const void*>(), { &createQualifiedProperty<const void*, TCHAR("const void*")>, &initQualifiedProperty<const void*> });

    FOR_EACH_SPECIAL_TYPES(CREATE_SPECIAL_PROPERTY);
}