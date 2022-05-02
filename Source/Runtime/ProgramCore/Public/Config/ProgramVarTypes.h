/*!
 * \file ProgramVarTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Delegates/Delegate.h"

#include <type_traits>

template <typename Type>
class ProgramConstant
{
private:
    static_assert(std::is_default_constructible_v<Type>, "The type for engine variable/constant should be default constructible");

public:
    using VarType = Type;

protected:
    VarType variable;

public:
    MAKE_TYPE_NONCOPY_NONMOVE(ProgramConstant)

    ProgramConstant() = default;
    ProgramConstant(const VarType &defaultVal)
        : variable(defaultVal)
    {}

    const VarType &get() const { return variable; }

    operator VarType() const { return variable; }
};

template <typename Type>
class ProgramGlobalVar : public ProgramConstant<Type>
{
private:
    using base = ProgramConstant<Type>;
    using VarType = typename base::VarType;
    using base::variable;

public:
    // void *(old, new)
    using VariableChanged = Event<ProgramGlobalVar<VarType>, VarType, VarType>;

private:
    VariableChanged onValueChanged;

public:
    ProgramGlobalVar() = default;
    ProgramGlobalVar(const VarType &defaultVal)
        : base(defaultVal)
    {}

    void set(const VarType &newValue)
    {
        if (!(variable == newValue))
        {
            VarType oldValue = variable;
            variable = newValue;
            onValueChanged.invoke(oldValue, newValue);
        }
    }

    void operator=(const VarType &newValue) { set(newValue); }

    VariableChanged &onConfigChanged() { return onValueChanged; }
};

template <typename Type, typename OwnerType>
class ProgramOwnedVar : public ProgramConstant<Type>
{
private:
    static_assert(std::is_class_v<OwnerType>, "Only class types are allowed as owner to allow modifications");
    using base = ProgramConstant<Type>;
    using VarType = typename base::VarType;
    using base::variable;

    friend OwnerType;

public:
    using VariableChanged = Event<ProgramOwnedVar<VarType, OwnerType>, VarType, VarType>;

private:
    VariableChanged onValueChanged;

private:
    void set(const VarType &newValue)
    {
        if (!(variable == newValue))
        {
            VarType oldValue = variable;
            variable = newValue;
            onValueChanged.invoke(oldValue, newValue);
        }
    }

    void operator=(const VarType &newValue) { set(newValue); }

public:
    ProgramOwnedVar() = default;
    ProgramOwnedVar(const VarType &defaultVal)
        : base(defaultVal)
    {}

    VariableChanged &onConfigChanged() { return onValueChanged; }
};