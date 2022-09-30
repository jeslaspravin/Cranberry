/*!
 * \file CoroutineUtilities.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoPaTConfig.h"

#include <coroutine>
#include <concepts>

COPAT_NS_INLINED
namespace copat
{

//////////////////////////////////////////////////////////////////////////
/// Coroutine type traits
//////////////////////////////////////////////////////////////////////////

template <typename AwaiterType, typename = void>
struct HasAwaitReady : std::false_type
{};

template <typename AwaiterType>
struct HasAwaitReady<AwaiterType, std::void_t<decltype(std::declval<AwaiterType>().await_ready())>>
{
    static constexpr bool value = std::is_same_v<decltype(std::declval<AwaiterType>().await_ready()), bool>;
};

template <typename AwaiterType>
constexpr bool HasAwaitReady_v = HasAwaitReady<AwaiterType>::value;

template <typename AwaiterType, typename = void>
struct HasAwaitSuspend : std::false_type
{};

template <typename AwaiterType>
struct HasAwaitSuspend<
    AwaiterType, std::void_t<decltype(std::is_member_function_pointer_v<decltype(&std::remove_cvref_t<AwaiterType>::await_suspend)>)>>
    : std::true_type
{};

template <typename AwaiterType>
constexpr bool HasAwaitSuspend_v = HasAwaitSuspend<AwaiterType>::value;

template <typename AwaiterType, typename = void>
struct HasAwaitResume : std::false_type
{};

template <typename AwaiterType>
struct HasAwaitResume<AwaiterType, std::void_t<decltype(std::declval<AwaiterType>().await_resume())>> : std::true_type
{};

template <typename AwaiterType>
constexpr bool HasAwaitResume_v = HasAwaitResume<AwaiterType>::value;

template <typename AwaiterType>
using AwaiterReturnType = decltype(std::declval<AwaiterType>().await_resume());

template <typename AwaiterType>
struct IsAwaiterType
{
    static constexpr bool value
        = HasAwaitReady<AwaiterType>::value && HasAwaitSuspend<AwaiterType>::value && HasAwaitResume<AwaiterType>::value;
};

template <typename AwaiterType>
constexpr bool IsAwaiterType_v = IsAwaiterType<AwaiterType>::value;

template <typename AwaiterType>
concept AwaiterTypeConcept = IsAwaiterType_v<AwaiterType>;

template <typename AwaitableType, typename = void>
struct HasPromiseType : std::false_type
{};
template <typename AwaitableType>
struct HasPromiseType<AwaitableType, std::void_t<typename std::remove_cvref_t<AwaitableType>::promise_type>> : std::true_type
{};

template <typename AwaitableType, typename = void>
struct HasCoAwait : std::false_type
{};
template <typename AwaitableType>
struct HasCoAwait<AwaitableType, std::void_t<decltype(std::declval<AwaitableType>().operator co_await())>> : std::true_type
{};

template <typename AwaitableType, typename = void>
struct HasAwaitTransformInPromise : std::false_type
{};
template <typename AwaitableType>
requires HasPromiseType<AwaitableType>::value struct HasAwaitTransformInPromise<
    AwaitableType, std::void_t<decltype(std::declval<typename std::remove_cvref_t<AwaitableType>::promise_type>()
                                            .await_transform(std::declval<std::remove_cvref_t<AwaitableType> &>()))>> : std::true_type
{};

/**
 * A type is considered awaitable even if it has only promise_type because these types must have co_await or await_transform defined inside
 * awaiting coroutine As of now IsAwaitableType and AwaitableTypeConcept is for using it with template types where awaitable is expected
 */
template <typename AwaitableType>
constexpr bool IsAwaitableType_v = HasPromiseType<AwaitableType>::value || HasCoAwait<AwaitableType>::value || IsAwaiterType_v<AwaitableType>;
template <typename AwaitableType>
concept AwaitableTypeConcept = IsAwaitableType_v<AwaitableType>;

template <typename AwaitableType>
struct GetAwaiterTypeFromCoawait
{
    using type = AwaitableType;
};
template <typename AwaitableType>
requires HasCoAwait<AwaitableType>::value struct GetAwaiterTypeFromCoawait<AwaitableType>
{
    using type = decltype(std::declval<AwaitableType>().operator co_await());
};

template <typename AwaitableType>
struct GetAwaiterType
{
    using type = std::remove_cvref_t<typename GetAwaiterTypeFromCoawait<AwaitableType>::type>;
};
template <typename AwaitableType>
requires HasAwaitTransformInPromise<AwaitableType>::value struct GetAwaiterType<AwaitableType>
{
    using AwaitableTypeName = std::remove_cvref_t<AwaitableType>;
    using type = std::remove_cvref_t<typename GetAwaiterTypeFromCoawait<
        decltype(std::declval<typename AwaitableTypeName::promise_type>().await_transform(std::declval<AwaitableTypeName &>()))>::type>;
};

template <typename AwaitableType>
using GetAwaiterType_t = GetAwaiterType<AwaitableType>::type;

//////////////////////////////////////////////////////////////////////////
/// Awaiter/Awaitable
//////////////////////////////////////////////////////////////////////////

struct NormalFuncAwaiter : std::suspend_never
{
    struct promise_type
    {
        constexpr NormalFuncAwaiter get_return_object() const noexcept { return {}; }
        constexpr std::suspend_never initial_suspend() const noexcept { return {}; }
        constexpr std::suspend_never final_suspend() const noexcept { return {}; }
        constexpr void return_void() const noexcept {}
        constexpr void unhandled_exception() const noexcept {}
    };
};

//////////////////////////////////////////////////////////////////////////
/// Helper types
//////////////////////////////////////////////////////////////////////////

/**
 * Stores coroutine L-Value and value return Type  R-Values are not supported
 */
template <class Type>
class CoroutineReturnStorage
{
public:
    using ReturnType = Type;
    using RefType = Type &;

    using value_type = ReturnType;
    using reference_type = RefType;
    static_assert(!std::is_rvalue_reference_v<Type>, "R-Value references are not supported return Type");

public:
    CoroutineReturnStorage() = default;
    CoroutineReturnStorage(CoroutineReturnStorage &&) = default;
    CoroutineReturnStorage(const CoroutineReturnStorage &) = default;
    CoroutineReturnStorage &operator=(CoroutineReturnStorage &&) = default;
    CoroutineReturnStorage &operator=(const CoroutineReturnStorage &) = default;

    template <typename AnyType>
    CoroutineReturnStorage(AnyType &&value)
        : returnValue(value)
    {}

    template <std::assignable_from<Type> AnyType>
    CoroutineReturnStorage &operator=(AnyType &&value)
    {
        returnValue = value;
        return *this;
    }

    constexpr operator Type() const noexcept { return returnValue; }

    [[nodiscard]] constexpr RefType get() noexcept { return returnValue; }

private:
    Type returnValue;
};

/**
 * L-Value reference specialization
 */
template <class Type>
class CoroutineReturnStorage<Type &>
{
public:
    using ReturnType = Type &;
    using RefType = ReturnType;

    using value_type = ReturnType;
    using reference_type = RefType;

public:
    CoroutineReturnStorage() = default;
    CoroutineReturnStorage(CoroutineReturnStorage &&) = default;
    CoroutineReturnStorage(const CoroutineReturnStorage &) = default;
    CoroutineReturnStorage &operator=(CoroutineReturnStorage &&) = default;
    CoroutineReturnStorage &operator=(const CoroutineReturnStorage &) = default;

    CoroutineReturnStorage(ReturnType value)
        : returnValue(&value)
    {}

    CoroutineReturnStorage &operator=(ReturnType value)
    {
        returnValue = &value;
        return *this;
    }

    constexpr operator ReturnType() const noexcept { return *returnValue; }

    [[nodiscard]] constexpr RefType get() const noexcept
    {
        COPAT_ASSERT(returnValue);
        return *returnValue;
    }

private:
    Type *returnValue = nullptr;
};

template <>
class CoroutineReturnStorage<void>
{
public:
    struct VoidType
    {};

    using ReturnType = VoidType;
    using RefType = VoidType;

    using reference_type = RefType;

public:
    CoroutineReturnStorage() = default;
    CoroutineReturnStorage(CoroutineReturnStorage &&) = default;
    CoroutineReturnStorage(const CoroutineReturnStorage &) = default;
    CoroutineReturnStorage &operator=(CoroutineReturnStorage &&) = default;
    CoroutineReturnStorage &operator=(const CoroutineReturnStorage &) = default;

    template <typename AnyType>
    CoroutineReturnStorage(AnyType &&value)
    {}
    template <typename AnyType>
    CoroutineReturnStorage &operator=(AnyType &&value)
    {
        return *this;
    }

    constexpr operator void() const noexcept {}

    constexpr RefType get() noexcept { return {}; }
};

/**
 * For using with shared pointers. Received address of coroutine frame and destroys it
 */
struct CoroutineDestroyer
{
    void operator()(void *ptr) const noexcept { std::coroutine_handle<>::from_address(ptr).destroy(); }
};

} // namespace copat