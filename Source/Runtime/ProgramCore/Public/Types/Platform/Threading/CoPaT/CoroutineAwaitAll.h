/*!
 * \file CoroutineAwaitAll.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoroutineUtilities.h"

#include <tuple>
#include <atomic>
#include <vector>

COPAT_NS_INLINED
namespace copat
{

namespace impl
{
class AwaitAllTasksCounter
{
private:
    std::atomic<u32> counter;
    std::coroutine_handle<> awaitingCoroutine;

public:
    AwaitAllTasksCounter(u32 initialCount)
        : counter(initialCount)
    {}
    AwaitAllTasksCounter(AwaitAllTasksCounter &&) = default;
    AwaitAllTasksCounter(const AwaitAllTasksCounter &) = default;
    AwaitAllTasksCounter &operator= (AwaitAllTasksCounter &&) = default;
    AwaitAllTasksCounter &operator= (const AwaitAllTasksCounter &) = default;

    void reset(u32 newCount) { counter.store(newCount, std::memory_order::release); }
    void setAwaitingCoroutine(std::coroutine_handle<> awaitingCoro)
    {
        COPAT_ASSERT(!awaitingCoroutine);
        awaitingCoroutine = awaitingCoro;
    }

    void release()
    {
        COPAT_ASSERT(counter.load(std::memory_order::acquire) > 0);
        if (counter.fetch_sub(1, std::memory_order::acq_rel) == 1 && awaitingCoroutine)
        {
            awaitingCoroutine.resume();
        }
    }
};
} // namespace impl

template <typename RetType>
class AwaitOneTask
{
public:
    struct PromiseType;
    using promise_type = PromiseType;
    using RetTypeStorage = CoroutineReturnStorage<RetType>;

private:
    std::coroutine_handle<PromiseType> ownerCoroutine;

public:
    AwaitOneTask(std::coroutine_handle<PromiseType> coro)
        : ownerCoroutine(coro)
    {}
    AwaitOneTask(AwaitOneTask &&) = default;
    AwaitOneTask(const AwaitOneTask &) = default;
    AwaitOneTask &operator= (AwaitOneTask &&) = default;
    AwaitOneTask &operator= (const AwaitOneTask &) = default;

    struct PromiseType
    {
    public:
        struct FinalSuspendAwaiter
        {
            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<PromiseType> h) const noexcept
            {
                COPAT_ASSERT(h.promise().waitCounter);
                h.promise().waitCounter->release();
            }
            constexpr void await_resume() const noexcept {}
        };

        RetTypeStorage returnStore;
        impl::AwaitAllTasksCounter *waitCounter = nullptr;

        AwaitOneTask get_return_object() noexcept { return AwaitOneTask(std::coroutine_handle<PromiseType>::from_promise(*this)); }
        // We do not suspend at start but when manually suspended in makeOneTaskAwaitable, This is to allow move Awaitable in r-value awaitables
        // So moved awaitable continue to exist until AwaitAllTasks is finished
        constexpr std::suspend_never initial_suspend() const noexcept { return {}; }
        constexpr FinalSuspendAwaiter final_suspend() const noexcept { return {}; }
        constexpr void unhandled_exception() const noexcept {}

        /**
         * Why not use return_value? For that we need both return_value and return_void defined but we cannot have both in same promise so went
         * with yield
         */
        template <typename ValType>
        FinalSuspendAwaiter yield_value(ValType &&retVal) noexcept
        {
            returnStore = RetTypeStorage{ std::forward<ValType>(retVal) };
            return final_suspend();
        }

        constexpr void return_void() const noexcept {}
    };

    /**
     * Sets counter waiting on this awaitable and resumes from initial_suspend. Counter gets released when final_suspend gets awaited
     */
    void setWaitCounter(impl::AwaitAllTasksCounter &counter) const
    {
        COPAT_ASSERT(ownerCoroutine && !ownerCoroutine.promise().waitCounter);
        ownerCoroutine.promise().waitCounter = &counter;
        ownerCoroutine.resume();
    }

    void destroyOwnerCoroutine()
    {
        ownerCoroutine.destroy();
        ownerCoroutine = nullptr;
    }

    constexpr RetTypeStorage::reference_type getReturnValue() const
    {
        if constexpr (!std::is_void_v<RetType>)
        {
            return ownerCoroutine.promise().returnStore.get();
        }
    }
};

template <typename AwaitingCollection>
class AwaitAllTasks;

/**
 * Specialization for variadic arguments using tuple
 */
template <AwaitableTypeConcept... Awaitables>
class AwaitAllTasks<std::tuple<Awaitables...>>
{
public:
    constexpr static const u32 AWAITABLES_COUNT = sizeof...(Awaitables);
    using AwaitingCollection = std::tuple<AwaitOneTask<AwaiterReturnType<GetAwaiterType_t<Awaitables>>>...>;
    using ResumeRetType = const AwaitingCollection &;

private:
    AwaitingCollection allAwaits;
    // Coroutine that awaits this gets resumed when this counter becomes 0
    impl::AwaitAllTasksCounter counter;

public:
    AwaitAllTasks()
        : counter(0)
    {}
    AwaitAllTasks(AwaitingCollection &&collection)
        : allAwaits(std::forward<AwaitingCollection>(collection))
        , counter(AWAITABLES_COUNT)
    {}

    AwaitAllTasks(AwaitAllTasks &&other)
        : allAwaits(std::move(other.allAwaits))
        , counter(std::move(other.counter))
    {}
    AwaitAllTasks &operator= (AwaitAllTasks &&other)
    {
        allAwaits = std::move(other.allAwaits);
        counter = std::move(other.counter);
        return *this;
    }

    ~AwaitAllTasks() { destroyAllAwaits(std::make_index_sequence<AWAITABLES_COUNT>{}); }

    // Delete copy
    AwaitAllTasks(const AwaitAllTasks &) = delete;
    AwaitAllTasks &operator= (const AwaitAllTasks &) = delete;

    // Should we refactor this into co_await binary operator as this type does not seems to be an awaitable?
    constexpr bool await_ready() const noexcept { return AWAITABLES_COUNT == 0; }
    constexpr void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
    {
        // It is okay we do not handle empty awaitable as await_ready will handle that
        counter.setAwaitingCoroutine(awaitingCoroutine);
        // Resume each of the Awaitable
        setAwaitsWaitCounter(std::make_index_sequence<AWAITABLES_COUNT>{});
    }
    ResumeRetType await_resume() const noexcept { return allAwaits; }

private:
    template <size_t Idx>
    void setAwaitWaitCounter()
    {
        std::get<Idx>(allAwaits).setWaitCounter(counter);
    }
    template <size_t... Indices>
    void setAwaitsWaitCounter(std::index_sequence<Indices...>)
    {
        (setAwaitWaitCounter<Indices>(), ...);
    }

    template <size_t Idx>
    void destroyAwait()
    {
        std::get<Idx>(allAwaits).destroyOwnerCoroutine();
    }
    template <size_t... Indices>
    void destroyAllAwaits(std::index_sequence<Indices...>)
    {
        (destroyAwait<Indices>(), ...);
    }
};

/**
 * Specialization for types that has size() and iterator
 */
template <
    template <typename AwaitableType, typename... OtherArgs> typename AwaitingCollectionType, AwaitableTypeConcept AwaitableType,
    typename... OtherArgs>
class AwaitAllTasks<AwaitingCollectionType<AwaitableType, OtherArgs...>>
{
public:
    using AwaitOneTaskType = AwaitOneTask<AwaiterReturnType<GetAwaiterType_t<AwaitableType>>>;
    using AwaitingCollection = AwaitingCollectionType<AwaitOneTaskType>;
    using ResumeRetType = const AwaitingCollection &;

private:
    AwaitingCollection allAwaits;
    // Coroutine that awaits this gets resumed when this counter becomes 0
    impl::AwaitAllTasksCounter counter;

public:
    AwaitAllTasks()
        : counter(0)
    {}
    AwaitAllTasks(AwaitingCollection &&collection)
        : allAwaits(std::forward<AwaitingCollection>(collection))
        , counter(u32(allAwaits.size()))
    {}

    AwaitAllTasks(AwaitAllTasks &&other)
        : allAwaits(std::move(other.allAwaits))
        , counter(std::move(other.counter))
    {}
    AwaitAllTasks &operator= (AwaitAllTasks &&other)
    {
        allAwaits = std::move(other.allAwaits);
        counter = std::move(other.counter);
        return *this;
    }

    ~AwaitAllTasks()
    {
        for (AwaitOneTaskType &awaitable : allAwaits)
        {
            awaitable.destroyOwnerCoroutine();
        }
    }

    // Delete copy
    AwaitAllTasks(const AwaitAllTasks &) = delete;
    AwaitAllTasks &operator= (const AwaitAllTasks &) = delete;

    // Should we refactor this into co_await binary operator as this type does not seems to be an awaitable?
    constexpr bool await_ready() const noexcept { return allAwaits.size() == 0; }
    constexpr void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
    {
        // It is okay we do not handle empty awaitable as await_ready will handle that
        counter.setAwaitingCoroutine(awaitingCoroutine);
        // Resume each of the Awaitable
        for (AwaitOneTaskType &awaitable : allAwaits)
        {
            awaitable.setWaitCounter(counter);
        }
    }
    ResumeRetType await_resume() const noexcept { return allAwaits; }
};

namespace impl
{

template <AwaitableTypeConcept Awaitable, typename RetType = AwaiterReturnType<GetAwaiterType_t<Awaitable>>>
requires (!std::is_void_v<RetType>)
AwaitOneTask<RetType> makeOneTaskAwaitable(Awaitable &&awaitable)
{
    if constexpr (std::is_lvalue_reference_v<Awaitable>)
    {
        // Since AwaitOneTask's promise do not suspend at initial_suspend, To allow capturing r-value awaitable
        co_await std::suspend_always{};
        co_yield co_await awaitable;
    }
    else
    {
        Awaitable localAwaitable(std::forward<Awaitable>(awaitable));
        co_await std::suspend_always{};
        co_yield co_await localAwaitable;
        // Keep awaitable around until this coro is destroyed
        co_return;
    }
}
template <AwaitableTypeConcept Awaitable, typename RetType = AwaiterReturnType<GetAwaiterType_t<Awaitable>>>
requires std::is_void_v<RetType>
AwaitOneTask<void> makeOneTaskAwaitable(Awaitable &&awaitable)
{
    if constexpr (std::is_lvalue_reference_v<Awaitable>)
    {
        // Since AwaitOneTask's promise do not suspend at initial_suspend, To allow capturing r-value awaitable
        co_await std::suspend_always{};
        co_await awaitable;
    }
    else
    {
        Awaitable localAwaitable(std::forward<Awaitable>(awaitable));
        co_await std::suspend_always{};
        co_await localAwaitable;
    }
}

} // namespace impl

/**
 * If piping the awaitAllTasks to waitOnAwaitable as r-value reference note that awaitAllTasks will be waitOnAwaitable scope
 * and gets destroyed when waitOnAwaitable returns. awaitAllTasks life time has to be managed by user of API
 */

template <AwaitableTypeConcept... Awaitables>
AwaitAllTasks<std::tuple<Awaitables...>> awaitAllTasks(Awaitables &&...awaitables)
{
    return AwaitAllTasks<std::tuple<Awaitables...>>{ std::tuple<AwaitOneTask<AwaiterReturnType<GetAwaiterType_t<Awaitables>>>...>{
        std::make_tuple(impl::makeOneTaskAwaitable<Awaitables>(std::forward<Awaitables>(awaitables))...) } };
}

template <AwaitableTypeConcept AwaitableType>
AwaitAllTasks<std::vector<AwaitableType>> awaitAllTasks(std::vector<AwaitableType> &awaitables)
{
    if (awaitables.empty())
    {
        return {};
    }

    using AwaiterRetType = AwaiterReturnType<GetAwaiterType_t<AwaitableType>>;
    std::vector<AwaitOneTask<AwaiterRetType>> allAwaits;
    allAwaits.reserve(awaitables.size());
    for (AwaitableType &awaitable : awaitables)
    {
        allAwaits.emplace_back(std::move(impl::makeOneTaskAwaitable(awaitable)));
    }
    return AwaitAllTasks<std::vector<AwaitableType>>{ std::move(allAwaits) };
}
template <AwaitableTypeConcept AwaitableType>
AwaitAllTasks<std::vector<AwaitableType>> awaitAllTasks(std::vector<AwaitableType> &&awaitables)
{
    if (awaitables.empty())
    {
        return {};
    }

    using AwaiterRetType = AwaiterReturnType<GetAwaiterType_t<AwaitableType>>;
    std::vector<AwaitOneTask<AwaiterRetType>> allAwaits;
    allAwaits.reserve(awaitables.size());
    for (AwaitableType &awaitable : awaitables)
    {
        allAwaits.emplace_back(std::move(impl::makeOneTaskAwaitable(std::move_if_noexcept<AwaitableType>(awaitable))));
    }
    awaitables.clear();
    return AwaitAllTasks<std::vector<AwaitableType>>{ std::move(allAwaits) };
}

} // namespace copat