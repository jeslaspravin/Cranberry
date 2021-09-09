
#include <type_traits>

template <typename T1, typename T2, typename Condition, T1 Value1, T2 Value2, typename ValType>
struct ConditionalValue;


template <typename T1, typename T2, typename Condition, T1 Value1, T2 Value2>
struct ConditionalValue<T1, T2, Condition, Value1, Value2, std::enable_if_t<Condition::value, T1>>
{
    static T1 value = Value1;
};

template <typename T1, typename T2, typename Condition, T1 Value1, T2 Value2>
struct ConditionalValue<T1, T2, Condition, Value1, Value2, std::enable_if_t<std::negation_v<Condition>, T2>>
{
    static T2 value = Value2;
};