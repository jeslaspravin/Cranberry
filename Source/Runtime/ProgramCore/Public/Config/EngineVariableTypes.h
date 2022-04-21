/*!
 * \file EngineVariableTypes.h
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
class EngineVar
{
private:
    static_assert(std::is_default_constructible_v<Type>, "The type for engine variable/constant should be default constructible");

public:
    using VarType = Type;

protected:
    VarType variable;

public:
    EngineVar(const EngineVar &) = delete;
    EngineVar(EngineVar &&) = delete;
    void operator=(const EngineVar &) = delete;
    void operator=(EngineVar &&) = delete;

    EngineVar() = default;
    EngineVar(const VarType &defaultVal)
        : variable(defaultVal)
    {}

    const VarType &get() const { return variable; }

    operator VarType() const { return variable; }
};

template <typename Type>
class EngineGlobalConfig : public EngineVar<Type>
{
private:
    using base = EngineVar<Type>;
    using VarType = typename base::VarType;
    using base::variable;

public:
    // void *(old, new)
    using GlobalConfigChanged = Event<EngineGlobalConfig<VarType>, VarType, VarType>;

private:
    GlobalConfigChanged onValueChanged;

public:
    EngineGlobalConfig() = default;
    EngineGlobalConfig(const VarType &defaultVal)
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

    GlobalConfigChanged &onConfigChanged() { return onValueChanged; }
};

template <typename Type, typename OwnerType>
class EngineConstant : public EngineVar<Type>
{
private:
    static_assert(std::is_class_v<OwnerType>, "Only class types are allowed as owner to allow modifications");
    using base = EngineVar<Type>;
    using VarType = typename base::VarType;
    using base::variable;

    friend OwnerType;

public:
    using ConstantChanged = Event<EngineConstant<VarType, OwnerType>, VarType, VarType>;

private:
    ConstantChanged onValueChanged;

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
    EngineConstant() = default;
    EngineConstant(const VarType &defaultVal)
        : base(defaultVal)
    {}

    ConstantChanged &onChanged() { return onValueChanged; }
};