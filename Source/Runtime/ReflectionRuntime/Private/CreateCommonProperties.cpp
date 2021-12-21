#include "ReflectionRuntimeModule.h"
#include "Property/Property.h"
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

#define CREATE_FUNDAMENTAL_PROPERTY(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createFundamentalProperty<TypeName, #TypeName>, nullptr }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName&>(), { &createFundamentalProperty<TypeName&, #TypeName" &">, nullptr }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName>(), { &createFundamentalProperty<const TypeName, "const "#TypeName>, nullptr }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName&>(), { &createFundamentalProperty<const TypeName&, "const "#TypeName" &">, nullptr }); \

#define CREATE_SPECIAL_PROPERTY(TypeName) \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName>(), { &createSpecialProperty<TypeName, #TypeName>, nullptr }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<TypeName&>(), { &createSpecialProperty<TypeName&, #TypeName" &">, nullptr }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName>(), { &createSpecialProperty<const TypeName, "const "#TypeName>, nullptr }); \
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const TypeName&>(), { &createSpecialProperty<const TypeName&, "const "#TypeName" &">, nullptr }); \

void ReflectionRuntimeModule::initCommonProperties() const
{
    FOR_EACH_CORE_TYPES(CREATE_FUNDAMENTAL_PROPERTY);
    // Register void as well
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<void>(), { &createFundamentalProperty<void, "void">, nullptr });
    IReflectionRuntimeModule::registerTypeFactory(typeInfoFrom<const void>(), { &createFundamentalProperty<const void, "const void">, nullptr });

    CREATE_SPECIAL_PROPERTY(String);
    CREATE_SPECIAL_PROPERTY(Color);
    CREATE_SPECIAL_PROPERTY(LinearColor);
    CREATE_SPECIAL_PROPERTY(Vector2D);
    CREATE_SPECIAL_PROPERTY(Vector3D);
    CREATE_SPECIAL_PROPERTY(Vector4D);
    CREATE_SPECIAL_PROPERTY(Matrix2);
    CREATE_SPECIAL_PROPERTY(Matrix3);
    CREATE_SPECIAL_PROPERTY(Matrix4);
    CREATE_SPECIAL_PROPERTY(Rotation);
    CREATE_SPECIAL_PROPERTY(Transform3D);
}