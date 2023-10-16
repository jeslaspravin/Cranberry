/*!
 * \file GenClassHierarchy.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Templates/TypeList.h"

/*
 * Scattered hierarchy and linear hierarchy can be used together
 * to create polymorphic behavior of same function(with almost same signature) but for different types.
 * These functions can be used to perform same operations on types provided by type list
 * and can be used from out side to invoke such an operation on each of those types/selected type
 *
 * Usage
 *
 * Here we are printing type info of each types in typelist without writing additional codes and using
 * both runtime and static polymorphisms
 *
 * using TestTL = TL::CreateFrom<ExperimentalEnginePBR, Vector2, Vector3, Vector4, Matrix2, Matrix3,
 * Matrix4>::type;
 *
 * template <typename PrintType>
 * class TestScatterGen
 * {
 * public:
 *     virtual void printType(TypeToType<PrintType>) const
 *     {
 *         LOG("ScatterGenTest", "Type Info : {}", *typeInfoFrom<PrintType>());
 *     }
 * };
 *
 * template <typename PrintType, typename BaseType>
 * class TestLinearGen : public BaseType
 * {
 * public:
 *     void printType(TypeToType<PrintType>) const
 *     {
 *         LOG("LinearGenTest", "Type Info : {}", *typeInfoFrom<PrintType>());
 *     }
 * };
 *
 * class ScatterGenClass : public GenScatteredHierarchy<TestTL, TestScatterGen>
 * {
 * public:
 *
 *     template <typename PrintType>
 *     void printType() const
 *     {
 *         const TestScatterGen<PrintType>& callObj = *this;
 *         callObj.printType(TypeToType<PrintType>());
 *     }
 * };
 *
 * class LinearGenClass : public GenLinearHierarchy<TL::Reverse<TestTL>::type, TestLinearGen,
 * ScatterGenClass>
 * {
 * public:
 *     using ScatterGenClass::printType;
 * };
 *
 * template <typename Type, typename PrintableType>
 * struct PrintTypeHelper
 * {
 *     void operator()(PrintableType* userData) const
 *     {
 *         userData->printType<Type>();
 *     }
 * };
 *
 * int main()
 * {
 *     // This will print all type's type info but calls TestScatterGen's Impl
 *     ScatterGenClass testScatter;
 *     TL::DoForEach<TestTL, PrintTypeHelper>::call(&testScatter);
 *
 *     // This will print all type's type info but calls TestLinearGen's Impl
 *     LinearGenClass testLinearGen;
 *     TL::DoForEach<TestTL, PrintTypeHelper>::call(&testLinearGen);
 * }
 *
 * Output(Not exact but similar) :
 * [ScatterGenTest] Type info[Name:class ExperimentalEnginePBR, Hash : xxyy, Qualifiers :( )]
 * [ScatterGenTest] Type info[Name:class Vector2, Hash : xxyy, Qualifiers :( )]
 * [ScatterGenTest] Type info[Name:class Vector3, Hash : xxyy, Qualifiers :( )]
 * [ScatterGenTest] Type info[Name:class Vector4, Hash : xxyy, Qualifiers :( )]
 * [ScatterGenTest] Type info[Name:class Matrix2, Hash : xxyy, Qualifiers :( )]
 * [ScatterGenTest] Type info[Name:class Matrix3, Hash : xxyy, Qualifiers :( )]
 * [ScatterGenTest] Type info[Name:class Matrix4, Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class ExperimentalEnginePBR,  Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class Vector2, Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class Vector3, Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class Vector4, Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class Matrix2, Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class Matrix3, Hash : xxyy, Qualifiers :( )]
 * [LinearGenTest] Type info[Name:class Matrix4, Hash : xxyy, Qualifiers :( )]
 *
 */

// Scatters inheritance across leaves of tree inheritance
template <typename TList, template <typename Type> typename InheritClassType>
class GenScatteredHierarchy;

template <typename LastType, template <typename Type> typename InheritClassType>
class GenScatteredHierarchy<TypeList<LastType, NullType>, InheritClassType> : public InheritClassType<LastType>
{
public:
    using ThisTypeList = TypeList<LastType, NullType>;
};

template <typename ThisType, typename NextType, template <typename Type> typename InheritClassType>
class GenScatteredHierarchy<TypeList<ThisType, NextType>, InheritClassType>
    : public InheritClassType<ThisType>
    , public GenScatteredHierarchy<NextType, InheritClassType>
{
public:
    using ThisTypeList = TypeList<ThisType, NextType>;
};

// Inherits all the types in order one one top of another
// GenLinearHierarchy inherits all the types in TList and last element in TList inherits from RootType
template <typename TList, template <typename Type, typename BaseType> typename InheritClassType, typename RootType = EmptyType>
class GenLinearHierarchy;

template <typename LastType, template <typename Type, typename BaseType> typename InheritClassType, typename RootType>
class GenLinearHierarchy<TypeList<LastType, NullType>, InheritClassType, RootType> : public InheritClassType<LastType, RootType>
{
public:
    using ThisTypeList = TypeList<LastType, NullType>;
};

template <typename ThisType, typename NextType, template <typename Type, typename BaseType> typename InheritClassType, typename RootType>
class GenLinearHierarchy<TypeList<ThisType, NextType>, InheritClassType, RootType>
    : public InheritClassType<ThisType, GenLinearHierarchy<NextType, InheritClassType, RootType>>
{
public:
    using ThisTypeList = TypeList<ThisType, NextType>;
};