#include "ReflectionRuntimeModule.h"
#include "ReflectionMacros.h"
#include "Types/TypesInfo.h"
#include "Math/CoreMathTypes.h"
#include "Types/Colors.h"
#include "Types/Transform3D.h"

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

#define CREATE_QUALIFIED_PROPERTIES(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName&>(), { &createQualifiedProperty<TypeName&, #TypeName" &">, &initQualifiedProperty<TypeName&> }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName*>(), { &createQualifiedProperty<TypeName*, #TypeName" *">, &initQualifiedProperty<TypeName*> }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName*>(), { &createQualifiedProperty<const TypeName*, "const "#TypeName" *">, &initQualifiedProperty<TypeName*> }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName&>(), { &createQualifiedProperty<const TypeName&, "const "#TypeName" &">, &initQualifiedProperty<TypeName*> });

#define CREATE_FUNDAMENTAL_PROPERTY(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createFundamentalProperty<TypeName, #TypeName>, nullptr }); \
    CREATE_QUALIFIED_PROPERTIES(TypeName)

#define CREATE_SPECIAL_PROPERTY(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createSpecialProperty<TypeName, #TypeName>, nullptr }); \
    CREATE_QUALIFIED_PROPERTIES(TypeName)

void ReflectionRuntimeModule::initCommonProperties() const
{
    FOR_EACH_CORE_TYPES(CREATE_FUNDAMENTAL_PROPERTY);
    // Register void as well
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<void>(), { &createFundamentalProperty<void, "void">, nullptr });
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<void*>(), { &createQualifiedProperty<void*, "void*">, &initQualifiedProperty<void*> });
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const void*>(), { &createQualifiedProperty<const void*, "const void*">, &initQualifiedProperty<const void*> });

    FOR_EACH_SPECIAL_TYPES(CREATE_SPECIAL_PROPERTY);
}