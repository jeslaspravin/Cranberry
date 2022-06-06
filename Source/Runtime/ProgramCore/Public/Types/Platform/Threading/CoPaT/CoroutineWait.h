/*!
 * \file CoroutineWait.h
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

#include <semaphore>

COPAT_NS_INLINED
namespace copat
{

namespace impl
{

template <typename RetType>
class WaitOnAwaitable
{
public:
    struct PromiseType;
    using promise_type = PromiseType;
    using RetTypeStorage = CoroutineReturnStorage<RetType>;

private:
    std::coroutine_handle<PromiseType> ownerCoroutine;

public:
    WaitOnAwaitable(std::coroutine_handle<PromiseType> owner)
        : ownerCoroutine(owner)
    {}
    WaitOnAwaitable(WaitOnAwaitable &&other)
        : ownerCoroutine(other.ownerCoroutine)
    {
        other.ownerCoroutine = nullptr;
    }
    WaitOnAwaitable &operator=(WaitOnAwaitable &&other)
    {
        ownerCoroutine = other.ownerCoroutine;
        other.ownerCoroutine = nullptr;
        return *this;
    }

    ~WaitOnAwaitable() noexcept
    {
        if (ownerCoroutine)
        {
            ownerCoroutine.destroy();
            ownerCoroutine = nullptr;
        }
    }

    // Delete copy and default initializations
    WaitOnAwaitable() = delete;
    WaitOnAwaitable(const WaitOnAwaitable &) = delete;
    WaitOnAwaitable &operator=(const WaitOnAwaitable &) = delete;

    struct PromiseType
    {
    public:
        struct FinalSuspendAwaiter
        {
            // We do not have to suspend at final if we are not returning any and so we do not have to keep coroutine frame alive
            // However we are suspending so that coroutine will be destroyed only once when WaitOnAwaitable destructor is executed
            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<PromiseType> h) const noexcept
            {
                COPAT_ASSERT(h.promise().waitingSemaphore);
                h.promise().waitingSemaphore->release();
            }
            constexpr void await_resume() const noexcept {}
        };

        RetTypeStorage returnStore;
        std::binary_semaphore *waitingSemaphore;

        WaitOnAwaitable get_return_object() noexcept { return WaitOnAwaitable(std::coroutine_handle<PromiseType>::from_promise(*this)); }
        constexpr std::suspend_always initial_suspend() const noexcept { return {}; }
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

    void startWait(std::binary_semaphore &inWaitingSemaphore)
    {
        COPAT_ASSERT(ownerCoroutine);

        // Clear the waiting semaphore first
        while (inWaitingSemaphore.try_acquire())
        {}
        ownerCoroutine.promise().waitingSemaphore = &inWaitingSemaphore;

        // Resume us as we must be suspended at initial_suspend for this step
        ownerCoroutine.resume();
    }

    constexpr RetTypeStorage::reference_type getReturnValue() const
    {
        if constexpr (!std::is_void_v<RetType>)
        {
            return ownerCoroutine.promise().returnStore.get();
        }
    }
};

template <AwaitableTypeConcept AwaitableType, typename RetType = AwaiterReturnType<GetAwaiterType_t<AwaitableType>>>
WaitOnAwaitable<RetType> awaitOnAwaitable(AwaitableType &awaitable)
{
    // Can we not yield on void? like promise_type::yield_value(void)?
    co_yield co_await awaitable;
}
template <AwaitableTypeConcept AwaitableType>
WaitOnAwaitable<void> awaitOnVoidAwaitable(AwaitableType &awaitable)
{
    co_await awaitable;
}

} // namespace impl

template <AwaitableTypeConcept AwaitableType, typename RetType = AwaiterReturnType<GetAwaiterType_t<AwaitableType>>>
requires(!std::is_void_v<RetType>) RetType waitOnAwaitable(AwaitableType &&awaitable)
{
    impl::WaitOnAwaitable<RetType> waitingOnAwaitable = impl::awaitOnAwaitable<AwaitableType, RetType>(awaitable);
    std::binary_semaphore semaphore{ 0 };
    waitingOnAwaitable.startWait(semaphore);
    semaphore.acquire();

    return waitingOnAwaitable.getReturnValue();
}

template <AwaitableTypeConcept AwaitableType, typename RetType = AwaiterReturnType<GetAwaiterType_t<AwaitableType>>>
requires std::is_void_v<RetType> RetType waitOnAwaitable(AwaitableType &&awaitable)
{
    impl::WaitOnAwaitable<RetType> waitingOnAwaitable = impl::awaitOnVoidAwaitable(awaitable);
    std::binary_semaphore semaphore{ 0 };
    waitingOnAwaitable.startWait(semaphore);
    semaphore.acquire();
}

} // namespace copat