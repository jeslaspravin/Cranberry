/*!
 * \file JobSystemCoroutine.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "SyncPrimitives.h"
#include "CoroutineUtilities.h"
#include "CoPaTTypes.h"

#include <atomic>
#include <concepts>
#include <mutex>

COPAT_NS_INLINED
namespace copat
{

class JobSystem;

template <EJobThreadType SwitchToThread>
struct SwitchJobThreadAwaiter
{

    JobSystem *enqToJobSystem = nullptr;

public:
    SwitchJobThreadAwaiter(JobSystem &jobSystem)
        : enqToJobSystem(&jobSystem)
    {}
    SwitchJobThreadAwaiter()
        : enqToJobSystem(JobSystem::get())
    {}

    // Even if nothing is awaiting it is still better to suspend as something might await on it after it if finished
    constexpr bool await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> h) const
    {
        COPAT_ASSERT(enqToJobSystem);
        enqToJobSystem->enqueueJob(h, SwitchToThread);
    }
    constexpr void await_resume() const noexcept {}
};

class JobSystemPromiseBase
{
private:
    std::coroutine_handle<> continuation = nullptr;
    // Should block any new continuation setup
    std::atomic_flag bBlockContinuation;

public:
    struct FinalSuspendAwaiter
    {
        // Even if nothing is awaiting it is still better to suspend as something might await on it after it if finished
        constexpr bool await_ready() const noexcept { return false; }
        template <std::derived_from<JobSystemPromiseBase> PromiseT>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseT> h) const noexcept
        {
            const bool bCoroutineSet = h.promise().bBlockContinuation.test_and_set(std::memory_order::acq_rel);
            if (bCoroutineSet)
            {
                COPAT_ASSERT(h.promise().continuation);
                return h.promise().continuation;
            }
            return std::noop_coroutine();
        }
        constexpr void await_resume() const noexcept {}
    };

    bool trySetContinuation(std::coroutine_handle<> newContinuation)
    {
        continuation = newContinuation;
        const bool bAlreadySetOrDone = bBlockContinuation.test_and_set(std::memory_order::acq_rel);
        if (bAlreadySetOrDone)
        {
            continuation = nullptr;
            return false;
        }
        return true;
    }

    FinalSuspendAwaiter final_suspend() noexcept { return {}; }
    constexpr void unhandled_exception() const {}
};

struct ContinuationEventChain
{
    std::coroutine_handle<> awaitingCoro;
    // No need to be atomic since lock ensures it is thread safe
    std::atomic<ContinuationEventChain *> pNext = nullptr;
};

/**
 * Multi continuation JobSystem Promise
 */
class JobSystemPromiseBaseMC
{
private:
    ContinuationEventChain eventChain;
    ContinuationEventChain *chainTailPtrCache = &eventChain;
    // Should block any new continuation setup
    std::atomic_flag bBlockContinuation;
    // We need lock here since coroutine might suspend even while we try set continuation
    SpinLock continuationLock;

public:
    ~JobSystemPromiseBaseMC()
    {
        chainTailPtrCache = nullptr;
        ContinuationEventChain *chainPtr = eventChain.pNext;
        eventChain.pNext = nullptr;
        while (chainPtr)
        {
            ContinuationEventChain *next = chainPtr->pNext;
            memDelete(chainPtr);
            chainPtr = next;
        }
    }

    struct FinalSuspendAwaiter
    {
        // Even if nothing is awaiting it is still better to suspend as something might await on it after it if finished
        constexpr bool await_ready() const { return false; }
        void await_suspend(std::coroutine_handle<JobSystemPromiseBaseMC> h) const
        {
            bool bAlreadySet = h.promise().bBlockContinuation.test_and_set(std::memory_order::release);
            COPAT_ASSERT(!bAlreadySet);

            std::scoped_lock<SpinLock> cs(h.promise().continuationLock);

            ContinuationEventChain *eventPtr = &h.promise().eventChain;
            while (eventPtr)
            {
                // Pull out first inline eventPtr
                if (eventPtr->awaitingCoro)
                {
                    eventPtr->awaitingCoro.resume();
                }
                eventPtr = eventPtr->pNext;
            }
            // Not deleting any ContinuationEventChain as trySetContinuation might be trying to set coroutine handle, Deleting is done when
            // coroutine is destroyed
        }
        constexpr void await_resume() const noexcept {}
    };

    bool trySetContinuation(std::coroutine_handle<> newContinuation)
    {
        std::coroutine_handle<JobSystemPromiseBaseMC> thisCoro = std::coroutine_handle<JobSystemPromiseBaseMC>::from_promise(*this);

        bool bAlreadyDone = bBlockContinuation.test(std::memory_order::acquire);
        // Just ensure coroutine is done, Is this correct?
        COPAT_ASSERT(bAlreadyDone && thisCoro.done());
        COPAT_ASSERT(chainTailPtrCache);

        if (bAlreadyDone)
        {
            // Do not suspend if it is done
            return false;
        }

        /**
         * We are just trying to append a continuation to current chainTailPtrCache's pNext and replace chainTailPtrCache with new
         * ContinuationEventChain in thread safe way
         */
        ContinuationEventChain *nextEventChain = memNew<ContinuationEventChain>();
        while (!bBlockContinuation.test(std::memory_order::acquire))
        {
            ContinuationEventChain *expectedValue = nullptr;
            // CAS on same chainTailPtrCache as eventually it will be changed in one of the threads
            if (chainTailPtrCache
                && chainTailPtrCache->pNext.compare_exchange_strong(expectedValue, nextEventChain, std::memory_order::acq_rel))
            {
                bAlreadyDone = bBlockContinuation.test(std::memory_order::acquire);

                /**
                 * Lock section here as we do not want FinalSuspendAwaiter::await_suspend to invalidate chainTailPtrCache
                 * Contention here will be blocked longer only if FinalSuspendAwaiter::await_suspend is happening other trySetContinuation will
                 * wait in CAS
                 */
                std::scoped_lock<SpinLock> cs(continuationLock);

                // Make sure this coroutine has not crossed the suspend point yet
                if (bAlreadyDone)
                {
                    // Release is enough since we are already in and no other trySetContinuation will be here yet
                    chainTailPtrCache->pNext.compare_exchange_strong(nextEventChain, expectedValue, std::memory_order::release);
                    memDelete(nextEventChain);
                    return false;
                }
                chainTailPtrCache->awaitingCoro = newContinuation;
                chainTailPtrCache = chainTailPtrCache->pNext;
                return true;
            }
        }
        memDelete(nextEventChain);
        return false;
    }

    FinalSuspendAwaiter final_suspend() noexcept { return {}; }
    constexpr void unhandled_exception() const {}
};

/**
 * This is an Awaiter type that if marked as return type of a function will be awaited from another coroutine until completion
 * EnqAtInitialSuspend - Determines if this coroutine with get executed through JobSystem at initial suspend, If false then coroutine gets
 * scheduled in appropriate thread at initial suspend InMainThread - Determines the thread in which initial_suspend queues this coroutine Task
 * that is not shareable ie) more than one copy cannot exist
 */
template <typename RetType, typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread>
class JobSystemTaskType
{
public:
    class PromiseType;

    using RetTypeStorage = CoroutineReturnStorage<RetType>;

private:
    // Coroutine that created this Awaitable/Awaiter
    std::coroutine_handle<PromiseType> ownerCoroutine;

public:
    JobSystemTaskType(std::coroutine_handle<PromiseType> owner)
        : ownerCoroutine(owner)
    {}
    JobSystemTaskType(JobSystemTaskType &&other)
        : ownerCoroutine(other.ownerCoroutine)
    {
        other.ownerCoroutine = nullptr;
    }
    JobSystemTaskType &operator= (JobSystemTaskType &&other)
    {
        if (ownerCoroutine)
        {
            ownerCoroutine.destroy();
        }
        ownerCoroutine = other.ownerCoroutine;
        other.ownerCoroutine = nullptr;
        return *this;
    }

    ~JobSystemTaskType() noexcept
    {
        if (ownerCoroutine)
        {
            ownerCoroutine.destroy();
            ownerCoroutine = nullptr;
        }
    }

    // Delete copy and default initializations
    JobSystemTaskType() = delete;
    JobSystemTaskType(const JobSystemTaskType &) = delete;
    JobSystemTaskType &operator= (const JobSystemTaskType &) = delete;

    class PromiseType : public BasePromiseType
    {
    public:
        RetTypeStorage returnStore;
        JobSystem *enqToJobSystem = nullptr;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        template <typename... FunctionParams>
        PromiseType(JobSystem &jobSystem, FunctionParams... params)
            : enqToJobSystem(&jobSystem)
        {}
        template <typename... FunctionParams>
        PromiseType(JobSystem *jobSystem, FunctionParams... params)
            : enqToJobSystem(jobSystem)
        {}

        PromiseType()
            : enqToJobSystem(JobSystem::get())
        {}

        JobSystemTaskType get_return_object() noexcept { return JobSystemTaskType{ std::coroutine_handle<PromiseType>::from_promise(*this) }; }
        auto initial_suspend() noexcept
        {
            if constexpr (EnqAtInitialSuspend)
            {
                COPAT_ASSERT(enqToJobSystem);
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread);
                return std::suspend_always{};
            }
            else
            {
                return std::suspend_never{};
            }
        }
        template <std::convertible_to<RetType> Type>
        void return_value(Type &&retVal)
        {
            returnStore = RetTypeStorage{ std::forward<Type>(retVal) };
        }
    };

    using promise_type = PromiseType;

    // If there is no coroutine then there is no need to wait as well
    bool await_ready() const { return !ownerCoroutine || ownerCoroutine.done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro)
    {
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    RetTypeStorage::reference_type await_resume() const { return ownerCoroutine.promise().returnStore.get(); }
};

/**
 * Specialization for void return type
 */
template <typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread>
class JobSystemTaskType<void, BasePromiseType, EnqAtInitialSuspend, EnqueueInThread>
{
public:
    class PromiseType;

private:
    // Coroutine that created this Awaitable/Awaiter
    std::coroutine_handle<PromiseType> ownerCoroutine;

public:
    JobSystemTaskType(std::coroutine_handle<PromiseType> owner)
        : ownerCoroutine(owner)
    {}
    JobSystemTaskType(JobSystemTaskType &&other)
        : ownerCoroutine(other.ownerCoroutine)
    {
        other.ownerCoroutine = nullptr;
    }
    JobSystemTaskType &operator= (JobSystemTaskType &&other)
    {
        if (ownerCoroutine)
        {
            ownerCoroutine.destroy();
        }
        ownerCoroutine = other.ownerCoroutine;
        other.ownerCoroutine = nullptr;
        return *this;
    }

    ~JobSystemTaskType() noexcept
    {
        if (ownerCoroutine)
        {
            ownerCoroutine.destroy();
            ownerCoroutine = nullptr;
        }
    }

    // Delete copy and default initializations
    JobSystemTaskType() = delete;
    JobSystemTaskType(const JobSystemTaskType &) = delete;
    JobSystemTaskType &operator= (const JobSystemTaskType &) = delete;

    class PromiseType : public BasePromiseType
    {
    public:
        JobSystem *enqToJobSystem = nullptr;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        template <typename... FunctionParams>
        PromiseType(JobSystem &jobSystem, FunctionParams...)
            : enqToJobSystem(&jobSystem)
        {}
        template <typename... FunctionParams>
        PromiseType(JobSystem *jobSystem, FunctionParams...)
            : enqToJobSystem(jobSystem)
        {}

        PromiseType()
            : enqToJobSystem(JobSystem::get())
        {}

        JobSystemTaskType get_return_object() noexcept { return JobSystemTaskType{ std::coroutine_handle<PromiseType>::from_promise(*this) }; }
        auto initial_suspend() noexcept
        {
            if constexpr (EnqAtInitialSuspend)
            {
                COPAT_ASSERT(enqToJobSystem);
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread);
                return std::suspend_always{};
            }
            else
            {
                return std::suspend_never{};
            }
        }
        constexpr void return_void() noexcept {}
    };

    using promise_type = PromiseType;

    // If there is no coroutine then there is no need to wait as well
    bool await_ready() const { return !ownerCoroutine || ownerCoroutine.done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro)
    {
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    constexpr void await_resume() const {}
};

/**
 * This is an Awaiter type that if marked as return type of a function will be awaited from another coroutine until completion
 * EnqAtInitialSuspend - Determines if this coroutine with get executed through JobSystem at initial suspend, If false then coroutine gets
 * scheduled in appropriate thread at initial suspend InMainThread - Determines the thread in which initial_suspend queues this coroutine Task
 * that is shareable and the coroutine handle is reference counted to ensure it gets destroyed after all Task copies gets destroyed
 */
template <typename RetType, typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread>
class JobSystemShareableTaskType
{
public:
    class PromiseType;

    using RetTypeStorage = CoroutineReturnStorage<RetType>;

private:
    // Coroutine that created this Awaitable/Awaiter
    std::shared_ptr<void> ownerCoroutinePtr;

public:
    JobSystemShareableTaskType(std::coroutine_handle<PromiseType> owner)
        : ownerCoroutinePtr(owner.address(), CoroutineDestroyer{})
    {}
    JobSystemShareableTaskType(JobSystemShareableTaskType &&) = default;
    JobSystemShareableTaskType &operator= (JobSystemShareableTaskType &&) = default;
    JobSystemShareableTaskType(const JobSystemShareableTaskType &) = default;
    JobSystemShareableTaskType &operator= (const JobSystemShareableTaskType &) = default;

    // Delete default initializations
    JobSystemShareableTaskType() = delete;

    class PromiseType : public BasePromiseType
    {
    public:
        RetTypeStorage returnStore;

        JobSystem *enqToJobSystem = nullptr;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        template <typename... FunctionParams>
        PromiseType(JobSystem &jobSystem, FunctionParams... params)
            : enqToJobSystem(&jobSystem)
        {}
        template <typename... FunctionParams>
        PromiseType(JobSystem *jobSystem, FunctionParams... params)
            : enqToJobSystem(jobSystem)
        {}

        PromiseType()
            : enqToJobSystem(JobSystem::get())
        {}

        JobSystemShareableTaskType get_return_object() noexcept
        {
            return JobSystemShareableTaskType{ std::coroutine_handle<PromiseType>::from_promise(*this) };
        }
        auto initial_suspend() noexcept
        {
            if constexpr (EnqAtInitialSuspend)
            {
                COPAT_ASSERT(enqToJobSystem);
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread);
                return std::suspend_always{};
            }
            else
            {
                return std::suspend_never{};
            }
        }
        template <std::convertible_to<RetType> Type>
        void return_value(Type &&retVal)
        {
            returnStore = RetTypeStorage{ std::forward<Type>(retVal) };
        }
    };

    using promise_type = PromiseType;

    // If there is no coroutine then there is no need to wait as well
    bool await_ready() const { return !ownerCoroutinePtr || std::coroutine_handle<>::from_address(ownerCoroutinePtr.get()).done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro)
    {
        std::coroutine_handle<PromiseType> ownerCoroutine = std::coroutine_handle<>::from_address(ownerCoroutinePtr.get());
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    RetTypeStorage::reference_type await_resume() const { return ownerCoroutinePtr.promise().returnStore.get(); }
};

/**
 * Specialization for void return type
 */
template <typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread>
class JobSystemShareableTaskType<void, BasePromiseType, EnqAtInitialSuspend, EnqueueInThread>
{
public:
    class PromiseType;

private:
    // Coroutine that created this Awaitable/Awaiter
    std::shared_ptr<void> ownerCoroutinePtr;

public:
    JobSystemShareableTaskType(std::coroutine_handle<PromiseType> owner)
        : ownerCoroutinePtr(owner.address(), CoroutineDestroyer{})
    {}
    JobSystemShareableTaskType(JobSystemShareableTaskType &&) = default;
    JobSystemShareableTaskType &operator= (JobSystemShareableTaskType &&) = default;
    JobSystemShareableTaskType(const JobSystemShareableTaskType &) = default;
    JobSystemShareableTaskType &operator= (const JobSystemShareableTaskType &) = default;

    // Delete default initializations
    JobSystemShareableTaskType() = delete;

    class PromiseType : public BasePromiseType
    {
    public:
        JobSystem *enqToJobSystem = nullptr;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        template <typename... FunctionParams>
        PromiseType(JobSystem &jobSystem, FunctionParams... params)
            : enqToJobSystem(&jobSystem)
        {}
        template <typename... FunctionParams>
        PromiseType(JobSystem *jobSystem, FunctionParams... params)
            : enqToJobSystem(jobSystem)
        {}

        PromiseType()
            : enqToJobSystem(JobSystem::get())
        {}

        JobSystemShareableTaskType get_return_object() noexcept
        {
            return JobSystemShareableTaskType{ std::coroutine_handle<PromiseType>::from_promise(*this) };
        }
        auto initial_suspend() noexcept
        {
            if constexpr (EnqAtInitialSuspend)
            {
                COPAT_ASSERT(enqToJobSystem);
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread);
                return std::suspend_always{};
            }
            else
            {
                return std::suspend_never{};
            }
        }
        constexpr void return_void() noexcept {}
    };

    using promise_type = PromiseType;

    // If there is no coroutine then there is no need to wait as well
    bool await_ready() const { return !ownerCoroutinePtr || std::coroutine_handle<>::from_address(ownerCoroutinePtr.get()).done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro)
    {
        std::coroutine_handle<PromiseType> ownerCoroutine = std::coroutine_handle<>::from_address(ownerCoroutinePtr.get());
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    constexpr void await_resume() const {}
};

// Some common typedefs

/**
 * Single awaitable, Auto enqueue task with no return type
 */
template <EJobThreadType EnqueueInThread>
using JobSystemEnqTask = JobSystemTaskType<void, JobSystemPromiseBase, true, EnqueueInThread>;
/**
 * Multi awaitable, Auto enqueue task with no return type
 */
template <EJobThreadType EnqueueInThread>
using JobSystemEnqTaskMultiAwait = JobSystemTaskType<void, JobSystemPromiseBaseMC, true, EnqueueInThread>;

/**
 * Single awaitable, Enqueue to Worker or Main thread specializations
 */
using JobSystemMainThreadTask = JobSystemEnqTask<EJobThreadType::MainThread>;
using JobSystemWorkerThreadTask = JobSystemEnqTask<EJobThreadType::WorkerThreads>;

/**
 * Multi awaitable, Enqueue to Worker or Main thread specializations
 */
using JobSystemMainThreadTaskMC = JobSystemEnqTaskMultiAwait<EJobThreadType::MainThread>;
using JobSystemWorkerThreadTaskMC = JobSystemEnqTaskMultiAwait<EJobThreadType::WorkerThreads>;

/**
 * Single awaitable, Manual await enqueue task with no return type
 */
using JobSystemTask = JobSystemTaskType<void, JobSystemPromiseBase, false, EJobThreadType::MainThread>;
/**
 * Multi awaitable, Manual await task with no return type
 */
using JobSystemTaskMC = JobSystemTaskType<void, JobSystemPromiseBaseMC, false, EJobThreadType::MainThread>;

/**
 * Single awaitable task with return type
 */
template <typename RetType, bool EnqAtInitSuspend, EJobThreadType EnqueueInThread>
using JobSystemReturnableTask = JobSystemTaskType<RetType, JobSystemPromiseBase, EnqAtInitSuspend, EnqueueInThread>;
/**
 * Multi awaitable task with return type
 */
template <typename RetType, bool EnqAtInitSuspend, EJobThreadType EnqueueInThread>
using JobSystemReturnableTaskMC = JobSystemTaskType<RetType, JobSystemPromiseBaseMC, EnqAtInitSuspend, EnqueueInThread>;

} // namespace copat