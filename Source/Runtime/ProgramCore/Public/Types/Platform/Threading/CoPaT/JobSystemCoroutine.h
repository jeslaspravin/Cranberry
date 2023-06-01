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

template <EJobThreadType SwitchToThread>
struct SwitchJobThreadAwaiter
{
public:
    SwitchJobThreadAwaiter() = default;

    // Even if nothing is awaiting it is still better to suspend as something might await on it after it if finished
    constexpr bool await_ready() const noexcept { return false; }
    template <JobSystemPromiseType PromiseType>
    void await_suspend(std::coroutine_handle<PromiseType> h) const noexcept
    {
        COPAT_ASSERT(h.promise().enqToJobSystem);
        h.promise().enqToJobSystem->enqueueJob(h, SwitchToThread, h.promise().jobPriority);
    }
    constexpr void await_resume() const noexcept {}
};

struct YieldAwaiter
{
public:
    YieldAwaiter() = default;

    // Even if nothing is awaiting it is still better to suspend as something might await on it after it if finished
    constexpr bool await_ready() const noexcept { return false; }
    template <JobSystemPromiseType PromiseType>
    void await_suspend(std::coroutine_handle<PromiseType> h) const noexcept
    {
        COPAT_ASSERT(h.promise().enqToJobSystem);
        h.promise().enqToJobSystem->enqueueJob(h, h.promise().enqToJobSystem->getCurrentThreadType(), h.promise().jobPriority);
    }
    constexpr void await_resume() const noexcept {}
};

class JobSystemPromiseBase
{
public:
    JobSystem *enqToJobSystem = nullptr;
    EJobPriority jobPriority = EJobPriority::Priority_Normal;

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

public:
    JobSystemPromiseBase(JobSystem *jobSystem, EJobPriority priority)
        : enqToJobSystem(jobSystem)
        , jobPriority(priority)
    {}

    bool trySetContinuation(std::coroutine_handle<> newContinuation) noexcept
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
    constexpr void unhandled_exception() const { COPAT_UNHANDLED_EXCEPT(); }
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
public:
    JobSystem *enqToJobSystem = nullptr;
    EJobPriority jobPriority = EJobPriority::Priority_Normal;

private:
    ContinuationEventChain eventChain;
    ContinuationEventChain *chainTailPtrCache = &eventChain;
    // Should block any new continuation setup
    std::atomic_flag bBlockContinuation;
    // We need lock here since coroutine might suspend even while we try set continuation
    SpinLock continuationLock;

public:
    JobSystemPromiseBaseMC(JobSystem *jobSystem, EJobPriority priority)
        : enqToJobSystem(jobSystem)
        , jobPriority(priority)
    {}

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
        void await_suspend(std::coroutine_handle<JobSystemPromiseBaseMC> h) const noexcept
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

    bool trySetContinuation(std::coroutine_handle<> newContinuation) noexcept
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
    constexpr void unhandled_exception() const { COPAT_UNHANDLED_EXCEPT(); }
};

/**
 * This is an Awaiter type that if marked as return type of a function will be awaited from another coroutine until completion
 * EnqAtInitialSuspend - Determines if this coroutine with get executed through JobSystem at initial suspend, If false then coroutine gets
 * scheduled in appropriate thread at initial suspend InMainThread - Determines the thread in which initial_suspend queues this coroutine Task
 * that is not shareable ie) more than one copy cannot exist
 */
template <typename RetType, typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
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
        using BasePromiseType::enqToJobSystem;
        using BasePromiseType::jobPriority;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        PromiseType(JobSystem &jobSystem, auto...)
            : BasePromiseType(&jobSystem, Priority)
        {}
        PromiseType(JobSystem *jobSystem, auto...)
            : BasePromiseType(jobSystem, Priority)
        {}
        PromiseType(JobSystem &jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(&jobSystem, priority)
        {}
        PromiseType(JobSystem *jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(jobSystem, priority)
        {}
        PromiseType(EJobPriority priority, auto...)
            : BasePromiseType(JobSystem::get(), priority)
        {}
        PromiseType()
            : BasePromiseType(JobSystem::get(), Priority)
        {}

        JobSystemTaskType get_return_object() noexcept { return JobSystemTaskType{ std::coroutine_handle<PromiseType>::from_promise(*this) }; }
        auto initial_suspend() noexcept
        {
            if constexpr (EnqAtInitialSuspend)
            {
                COPAT_ASSERT(enqToJobSystem);
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread, jobPriority);
                return std::suspend_always{};
            }
            else
            {
                return std::suspend_never{};
            }
        }
        template <std::convertible_to<RetType> Type>
        void return_value(Type &&retVal) noexcept
        {
            returnStore = RetTypeStorage{ std::forward<Type>(retVal) };
        }
    };

    using promise_type = PromiseType;

    // If there is no coroutine then there is no need to wait as well
    bool await_ready() const noexcept { return !ownerCoroutine || ownerCoroutine.done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro) noexcept
    {
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    RetTypeStorage::reference_type await_resume() const noexcept { return ownerCoroutine.promise().returnStore.get(); }
};

/**
 * Specialization for void return type
 */
template <typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
class JobSystemTaskType<void, BasePromiseType, EnqAtInitialSuspend, EnqueueInThread, Priority>
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
        using BasePromiseType::enqToJobSystem;
        using BasePromiseType::jobPriority;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        PromiseType(JobSystem &jobSystem, auto...)
            : BasePromiseType(&jobSystem, Priority)
        {}
        PromiseType(JobSystem *jobSystem, auto...)
            : BasePromiseType(jobSystem, Priority)
        {}
        PromiseType(JobSystem &jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(&jobSystem, priority)
        {}
        PromiseType(JobSystem *jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(jobSystem, priority)
        {}
        PromiseType(EJobPriority priority, auto...)
            : BasePromiseType(JobSystem::get(), priority)
        {}
        PromiseType()
            : BasePromiseType(JobSystem::get(), Priority)
        {}

        JobSystemTaskType get_return_object() noexcept { return JobSystemTaskType{ std::coroutine_handle<PromiseType>::from_promise(*this) }; }
        auto initial_suspend() noexcept
        {
            if constexpr (EnqAtInitialSuspend)
            {
                COPAT_ASSERT(enqToJobSystem);
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread, jobPriority);
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
    bool await_ready() const noexcept { return !ownerCoroutine || ownerCoroutine.done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro) noexcept
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
template <typename RetType, typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
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
        using BasePromiseType::enqToJobSystem;
        using BasePromiseType::jobPriority;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        PromiseType(JobSystem &jobSystem, auto...)
            : BasePromiseType(&jobSystem, Priority)
        {}
        PromiseType(JobSystem *jobSystem, auto...)
            : BasePromiseType(jobSystem, Priority)
        {}
        PromiseType(JobSystem &jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(&jobSystem, priority)
        {}
        PromiseType(JobSystem *jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(jobSystem, priority)
        {}
        PromiseType(EJobPriority priority, auto...)
            : BasePromiseType(JobSystem::get(), priority)
        {}
        PromiseType()
            : BasePromiseType(JobSystem::get(), Priority)
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
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread, jobPriority);
                return std::suspend_always{};
            }
            else
            {
                return std::suspend_never{};
            }
        }
        template <std::convertible_to<RetType> Type>
        void return_value(Type &&retVal) noexcept
        {
            returnStore = RetTypeStorage{ std::forward<Type>(retVal) };
        }
    };

    using promise_type = PromiseType;

    // If there is no coroutine then there is no need to wait as well
    bool await_ready() const noexcept { return !ownerCoroutinePtr || std::coroutine_handle<>::from_address(ownerCoroutinePtr.get()).done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro) noexcept
    {
        std::coroutine_handle<PromiseType> ownerCoroutine = std::coroutine_handle<>::from_address(ownerCoroutinePtr.get());
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    RetTypeStorage::reference_type await_resume() const noexcept { return ownerCoroutinePtr.promise().returnStore.get(); }
};

/**
 * Specialization for void return type
 */
template <typename BasePromiseType, bool EnqAtInitialSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
class JobSystemShareableTaskType<void, BasePromiseType, EnqAtInitialSuspend, EnqueueInThread, Priority>
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
        using BasePromiseType::enqToJobSystem;
        using BasePromiseType::jobPriority;

    public:
        /**
         * User provided JobSystem. First argument of coroutine function must be JobSystem & or JobSystem *
         */
        PromiseType(JobSystem &jobSystem, auto...)
            : BasePromiseType(&jobSystem, Priority)
        {}
        PromiseType(JobSystem *jobSystem, auto...)
            : BasePromiseType(jobSystem, Priority)
        {}
        PromiseType(JobSystem &jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(&jobSystem, priority)
        {}
        PromiseType(JobSystem *jobSystem, EJobPriority priority, auto...)
            : BasePromiseType(jobSystem, priority)
        {}
        PromiseType(EJobPriority priority, auto...)
            : BasePromiseType(JobSystem::get(), priority)
        {}
        PromiseType()
            : BasePromiseType(JobSystem::get(), Priority)
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
                enqToJobSystem->enqueueJob(std::coroutine_handle<PromiseType>::from_promise(*this), EnqueueInThread, jobPriority);
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
    bool await_ready() const noexcept { return !ownerCoroutinePtr || std::coroutine_handle<>::from_address(ownerCoroutinePtr.get()).done(); }
    template <typename PromiseT>
    bool await_suspend(std::coroutine_handle<PromiseT> awaitingAtCoro) noexcept
    {
        std::coroutine_handle<PromiseType> ownerCoroutine = std::coroutine_handle<>::from_address(ownerCoroutinePtr.get());
        return ownerCoroutine.promise().trySetContinuation(awaitingAtCoro);
    }
    constexpr void await_resume() const {}
};

//////////////////////////////////////////////////////////////////////////
/// Some common typedefs
//////////////////////////////////////////////////////////////////////////

// No returns

/**
 * Single awaitable task with no return type
 */
template <bool EnqAtInitSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
using JobSystemNoReturnTask = JobSystemTaskType<void, JobSystemPromiseBase, EnqAtInitSuspend, EnqueueInThread, Priority>;
/**
 * Multi awaitable task with no return type
 */
template <bool EnqAtInitSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
using JobSystemNoReturnTaskMC = JobSystemTaskType<void, JobSystemPromiseBaseMC, EnqAtInitSuspend, EnqueueInThread, Priority>;

/**
 * Single awaitable, Auto enqueue task with no return type
 */
template <EJobThreadType EnqueueInThread, EJobPriority Priority>
using JobSystemEnqTask = JobSystemNoReturnTask<true, EnqueueInThread, Priority>;
/**
 * Multi awaitable, Auto enqueue task with no return type
 */
template <EJobThreadType EnqueueInThread, EJobPriority Priority>
using JobSystemEnqTaskMC = JobSystemNoReturnTaskMC<true, EnqueueInThread, Priority>;

/**
 * Single awaitable, Enqueue to Worker or Main thread specializations, no return type
 */
using JobSystemMainThreadTask = JobSystemEnqTask<EJobThreadType::MainThread, EJobPriority::Priority_Normal>;
using JobSystemWorkerThreadTask = JobSystemEnqTask<EJobThreadType::WorkerThreads, EJobPriority::Priority_Normal>;

/**
 * Multi awaitable, Enqueue to Worker or Main thread specializations, no return type
 */
using JobSystemMainThreadTaskMC = JobSystemEnqTaskMC<EJobThreadType::MainThread, EJobPriority::Priority_Normal>;
using JobSystemWorkerThreadTaskMC = JobSystemEnqTaskMC<EJobThreadType::WorkerThreads, EJobPriority::Priority_Normal>;

/**
 * Single awaitable, Manual await task with no return type
 */
using JobSystemTask = JobSystemNoReturnTask<false, EJobThreadType::MainThread, EJobPriority::Priority_Normal>;
/**
 * Multi awaitable, Manual await task with no return type
 */
using JobSystemTaskMC = JobSystemNoReturnTaskMC<false, EJobThreadType::MainThread, EJobPriority::Priority_Normal>;

// With return type

/**
 * Single awaitable task with return type
 */
template <typename RetType, bool EnqAtInitSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
using JobSystemReturnableTask = JobSystemTaskType<RetType, JobSystemPromiseBase, EnqAtInitSuspend, EnqueueInThread, Priority>;
/**
 * Multi awaitable task with return type
 */
template <typename RetType, bool EnqAtInitSuspend, EJobThreadType EnqueueInThread, EJobPriority Priority>
using JobSystemReturnableTaskMC = JobSystemTaskType<RetType, JobSystemPromiseBaseMC, EnqAtInitSuspend, EnqueueInThread, Priority>;

} // namespace copat