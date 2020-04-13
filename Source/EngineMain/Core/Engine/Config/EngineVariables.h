#pragma once

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
    EngineVar(const EngineVar&) = delete;
    EngineVar(EngineVar&&) = delete;
    void operator=(const EngineVar&) = delete;
    void operator=(EngineVar&&) = delete;

    EngineVar() = default;
    EngineVar(const VarType& defaultVal) : variable(defaultVal){}

    VarType get() const
    {
        return variable;
    }
};

template <typename Type>
class EngineGlobalConfig : public EngineVar<Type>
{
private:
    using base = EngineVar<Type>;
    using VarType = base::VarType;
    using base::variable;
public:
    EngineGlobalConfig() = default;
    EngineGlobalConfig(const VarType& defaultVal) : base(defaultVal){}

    void set(const VarType& newValue)
    {
        variable = newValue;
    }
};

template <typename Type,typename OwnerType>
class EngineConstant : public EngineVar<Type>
{
private:
    static_assert(std::is_class_v<OwnerType>, "Only class types are allowed as owner to allow modifications");
    using base = EngineVar<Type>;
    using VarType = base::VarType;
    using base::variable;

    friend OwnerType;

private:
    void set(const VarType& newValue)
    {
        variable = newValue;
    }

    VarType* operator*()
    {
        return &variable;
    }
public:
    EngineConstant() = default;
    EngineConstant(const VarType & defaultVal) : base(defaultVal) {}
};