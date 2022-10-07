//$file${include::qxk.hpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${include::qxk.hpp}
//
// This code has been generated by QM 5.2.2 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #    : LicenseRef-QL-dual
// Issued to    : Any user of the QP/C++ real-time embedded framework
// Framework(s) : qpcpp
// Support ends : 2023-12-31
// License scope:
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//
//$endhead${include::qxk.hpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//! @file
//! @brief QXK/C++ preemptive extended (blocking) kernel, platform-independent
//! public interface.

#ifndef QP_INC_QXK_HPP_
#define QP_INC_QXK_HPP_

//============================================================================
// QF customization for QXK -- data members of the QActive class...

// QXK event-queue used for AOs
#define QF_EQUEUE_TYPE      QEQueue

// QXK OS-object used to store the private stack pointer for extended threads.
// (The private stack pointer is NULL for basic-threads).
#define QF_OS_OBJECT_TYPE   void*

// QXK thread type used to store the private Thread-Local Storage pointer.
#define QF_THREAD_TYPE      void*

//! Access Thread-Local Storage (TLS) and cast it on the given `type_`
#define QXK_TLS(type_) (static_cast<type_>(QXK_current()->m_thread))

//============================================================================
#include "qequeue.hpp"  // QXK kernel uses the native QF event queue
#include "qmpool.hpp"   // QXK kernel uses the native QF memory pool
#include "qf.hpp"       // QF framework integrates directly with QXK

//============================================================================
//$declare${QXK::QXTHREAD_NO_TIMEOUT} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QXK::QXTHREAD_NO_TIMEOUT} ................................................
//! No-timeout when blocking on semaphores, mutextes, and queues
constexpr std::uint_fast16_t QXTHREAD_NO_TIMEOUT {0U};

} // namespace QP
//$enddecl${QXK::QXTHREAD_NO_TIMEOUT} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$declare${QXK::QXK-base} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {
namespace QXK {

//${QXK::QXK-base::onIdle} ...................................................
//! QXK idle callback (customized in BSPs for QXK)
//!
//! @details
//! QXK::onIdle() is called continously by the QXK idle loop. This
//! callback gives the application an opportunity to enter a power-saving
//! CPU mode, or perform some other idle processing.
//!
//! @note
//! QXK::onIdle() is invoked with interrupts enabled and must also return
//! with interrupts enabled.
//!
//! @sa
//! QK::onIdle(), QXK::onIdle()
void onIdle();

//${QXK::QXK-base::schedLock} ................................................
//! QXK selective scheduler lock
//!
//! @details
//! This function locks the QXK scheduler to the specified ceiling.
//!
//! @param[in]   ceiling    priority ceiling to which the QXK scheduler
//!                         needs to be locked
//!
//! @returns
//! The previous QXK Scheduler lock status, which is to be used to unlock
//! the scheduler by restoring its previous lock status in
//! QXK::schedUnlock().
//!
//! @note
//! QXK::schedLock() must be always followed by the corresponding
//! QXK::schedUnlock().
//!
//! @sa QXK::schedUnlock()
//!
//! @usage
//! The following example shows how to lock and unlock the QXK scheduler:
//! @include qxk_lock.cpp
QSchedStatus schedLock(std::uint_fast8_t const ceiling) noexcept;

//${QXK::QXK-base::schedUnlock} ..............................................
//! QXK selective scheduler unlock
//!
//! @details
//! This function unlocks the QXK scheduler to the previous status.
//!
//! @param[in]   stat       previous QXK Scheduler lock status returned
//!                         from QXK::schedLock()
//! @note
//! A QXK scheduler can be locked from both basic threads (AOs) and
//! extended threads and the scheduler locks can nest.
//!
//! @note
//! QXK::schedUnlock() must always follow the corresponding
//! QXK::schedLock().
//!
//! @sa QXK::schedLock()
//!
//! @usage
//! The following example shows how to lock and unlock the QXK scheduler:
//! @include qxk_lock.cpp
void schedUnlock(QSchedStatus const stat) noexcept;

//${QXK::QXK-base::Timeouts} .................................................
//! timeout signals for extended threads
enum Timeouts : enum_t {
    DELAY_SIG = 1,
    TIMEOUT_SIG
};

} // namespace QXK
} // namespace QP
//$enddecl${QXK::QXK-base} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$declare${QXK::QXThread} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QXK::QXThread} ...........................................................
//! Extended (blocking) thread of the QXK preemptive kernel
//!
//! @details
//! QP::QXThread represents the extended (blocking) thread of the QXK kernel.
//! Each blocking thread in the application must be represented by the
//! corresponding QP::QXThread instance
//!
//! @note
//! Typically QP::QXThread is instantiated directly in the application code.
//! The customization of the thread occurs in the constructor, where you
//! provide the thread-handler function as the parameter.
//!
//! @sa QP::QActive
//!
//! @usage
//! The following example illustrates how to instantiate and use an extended
//! thread in your application.
//! @include qxk_thread.cpp
//!
class QXThread : public QP::QActive {
private:

    //! time event to handle blocking timeouts
    QTimeEvt m_timeEvt;

    // friends...
    friend class QXSemaphore;
    friend class QXMutex;

public:

    //! public constructor
    //!
    //! @details
    //! Performs the first step of QXThread initialization by assigning the
    //! thread-handler function and the tick rate at which it will handle
    //! the timeouts.
    //!
    //! @param[in]     handler  the thread-handler function
    //! @param[in]     tickRate the ticking rate associated with this thread
    //!                for timeouts in this thread (see QXThread::delay() and
    //!                TICK_X())
    //! @note
    //! Must be called only ONCE before QXThread::start().
    QXThread(
        QXThreadHandler const handler,
        std::uint_fast8_t const tickRate = 0U) noexcept;

    //! obtain the time event
    QTimeEvt const * getTimeEvt() const noexcept {
        return &m_timeEvt;
    }

    //! delay (block) the current extended thread for a specified # ticks
    //!
    //! @details
    //! Blocking delay for the number of clock tick at the associated
    //! tick rate.
    //!
    //! @param[in]  nTicks    number of clock ticks (at the associated rate)
    //!                       to wait for the event to arrive.
    //! @returns
    //! 'true' if the delay expired and `false` if it was cancelled
    //! by call to QTimeEvt::delayCancel()
    //!
    //! @note
    //! For the delay to work, the TICK_X() macro needs to be called
    //! periodically at the associated clock tick rate.
    //!
    //! @sa
    //! QP::QXThread, QTimeEvt::tick_()
    static bool delay(std::uint_fast16_t const nTicks) noexcept;

    //! cancel the delay
    //!
    //! @details
    //! Cancel the blocking delay and cause return from the QXThread::delay()
    //! function.
    //!
    //! @returns
    //! "true" if the thread was actually blocked on QXThread::delay() and
    //! "false" otherwise.
    bool delayCancel() noexcept;

    //! Get a message from the private message queue (block if no messages)
    //!
    //! @details
    //! The QXThread::queueGet() operation allows the calling extended thread
    //! to receive QP events (see QP::QEvt) directly into its own built-in
    //! event queue from an ISR, basic thread (AO), or another extended thread.
    //!
    //! If QXThread::queueGet() is called when no events are present in the
    //! thread's private event queue, the operation blocks the current
    //! extended thread until either an event is received, or a user-specified
    //! timeout expires.
    //!
    //! @param[in]  nTicks  number of clock ticks (at the associated rate)
    //!                     to wait for the event to arrive. The value of
    //!                     QP::QXTHREAD_NO_TIMEOUT indicates that no timeout
    //!                     will occur and the queue will block indefinitely.
    //! @returns
    //! A pointer to the event. If the pointer is not nullptr, the event
    //! was delivered. Otherwise the event pointer of nullptr indicates that
    //! the queue has timed out.
    static QEvt const * queueGet(std::uint_fast16_t const nTicks = QXTHREAD_NO_TIMEOUT) noexcept;

    //! Overrides QHsm::init()
    void init(
        void const * const e,
        std::uint_fast8_t const qs_id) override;

    //! Overrides QHsm::init()
    void init(std::uint_fast8_t const qs_id) override;

    //! Overrides QHsm::dispatch()
    void dispatch(
        QEvt const * const e,
        std::uint_fast8_t const qs_id) override;

    //! Starts execution of an extended thread and registers the thread
    //! with the framework
    //!
    //! @details
    //! Starts execution of an extended thread and registers it with the
    //! framework. The extended thread becomes ready-to-run immediately and
    //! is scheduled if the QXK is already running.
    //!
    //! @param[in] prioSpec priority specification at which to start the
    //!                     extended thread
    //! @param[in] qSto     pointer to the storage for the ring buffer of
    //!                     the event queue. This cold be NULL, if this
    //!                     extended thread does not use the built-in
    //!                     event queue.
    //! @param[in] qLen     length of the event queue [in events],
    //!                     or zero if queue not used
    //! @param[in] stkSto   pointer to the stack storage (must be provided)
    //! @param[in] stkSize  stack size [in bytes] (must not be zero)
    //! @param[in] par      pointer to an extra parameter (might be NULL)
    //!
    //! @usage
    //! The following example shows starting an extended thread:
    //! @include qxk_start.cpp
    void start(
        QPrioSpec const prioSpec,
        QEvt const * * const qSto,
        std::uint_fast16_t const qLen,
        void * const stkSto,
        std::uint_fast16_t const stkSize,
        void const * const par) override;

    //! Overloaded start function (no initialization event)
    void start(
        QPrioSpec const prioSpec,
        QEvt const * * const qSto,
        std::uint_fast16_t const qLen,
        void * const stkSto,
        std::uint_fast16_t const stkSize) override
    {
        this->start(prioSpec, qSto, qLen, stkSto, stkSize, nullptr);
    }

    //! Posts an event `e` directly to the event queue of the extended
    //! thread using the First-In-First-Out (FIFO) policy
    //!
    //! @details
    //! Extended threads can be configured (in QXThread::start()) to have
    //! a private event queue. In that case, QP events (see QP::QEvt) can
    //! be asynchronously posted or published to the extended thread.
    //! The thread can wait (and block) on its queue and then it can
    //! process the delivered event.
    //!
    //! @param[in] e      pointer to the event to be posted
    //! @param[in] margin number of required free slots in the queue
    //!                   after posting the event. The special value
    //!                   QF::NO_MARGIN means that this function will
    //!                   assert if posting fails.
    //! @param[in] sender pointer to a sender object (used in QS only)
    //!
    //! @returns
    //! 'true' (success) if the posting succeeded (with the provided margin)
    //! and 'false' (failure) when the posting fails.
    //!
    //! @attention
    //! Should be called only via the macro POST() or POST_X().
    //!
    //! @note
    //! The QF::NO_MARGIN value of the `margin` parameter is special and
    //! denotes situation when the post() operation is assumed to succeed
    //! (event delivery guarantee). An assertion fires, when the event cannot
    //! be delivered in this case.
    bool post_(
        QEvt const * const e,
        std::uint_fast16_t const margin,
        void const * const sender) noexcept override;

    //! Posts an event directly to the event queue of the extended thread
    //! using the Last-In-First-Out (LIFO) policy
    //!
    //! @details
    //! Last-In-First-Out (LIFO) policy is not supported for extended threads.
    //!
    //! @param[in]  e  pointer to the event to post to the queue
    //!
    //! @sa
    //! QXThread::post_(), QActive::postLIFO_()
    void postLIFO(QEvt const * const e) noexcept override;

private:

    //! Block the extended thread
    //!
    //! @details
    //! Internal implementation of blocking the given extended thread.
    //!
    //! @note
    //! Must be called from within a critical section
    void block_() const noexcept;

    //! Unblock the extended thread
    //!
    //! @details
    //! Internal implementation of unblocking the given extended thread.
    //!
    //! @note
    //! must be called from within a critical section
    void unblock_() const noexcept;

    //! Arm the private time event
    //!
    //! @details
    //! Internal implementation of arming the private time event for
    //! a given timeout at a given system tick rate.
    //!
    //! @note
    //! Must be called from within a critical section
    void teArm_(
        enum_t const sig,
        std::uint_fast16_t const nTicks) noexcept;

    //! Disarm the private time event
    //!
    //! @details
    //! Internal implementation of disarming the private time event.
    //!
    //! @note
    //! Must be called from within a critical section
    bool teDisarm_() noexcept;
}; // class QXThread

} // namespace QP
//$enddecl${QXK::QXThread} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$declare${QXK::QXSemaphore} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QXK::QXSemaphore} ........................................................
//! Counting Semaphore of the QXK preemptive kernel
//!
//! @details
//! QP::QXSemaphore is a blocking mechanism intended primarily for signaling
//! @ref QP::QXThread "extended threads". The semaphore is initialized with
//! the maximum count (see QP::QXSemaphore::init()), which allows you to
//! create a binary semaphore (when the maximum count is 1) and
//! counting semaphore when the maximum count is > 1.
//!
//! @usage
//! The following example illustrates how to instantiate and use the semaphore
//! in your application.
//! @include qxk_sema.cpp
//!
class QXSemaphore {
private:

    //! set of extended threads waiting on this semaphore
    QPSet m_waitSet;

    //! semaphore up-down counter
    std::uint16_t volatile m_count;

    //! maximum value of the semaphore counter
    std::uint16_t m_max_count;

public:

    //! initialize the counting semaphore
    //!
    //! @details
    //! Initializes a semaphore with the specified count and maximum count.
    //! If the semaphore is used for resource sharing, both the initial count
    //! and maximum count should be set to the number of identical resources
    //! guarded by the semaphore. If the semaphore is used as a signaling
    //! mechanism, the initial count should set to 0 and maximum count to 1
    //! (binary semaphore).
    //!
    //! @param[in]     count  initial value of the semaphore counter
    //! @param[in]     max_count  maximum value of the semaphore counter.
    //!                The purpose of the max_count is to limit the counter
    //!                so that the semaphore cannot unblock more times than
    //!                the maximum.
    //! @note
    //! QXSemaphore::init() must be called **before** the semaphore can be
    //! used (signaled or waited on).
    void init(
        std::uint_fast16_t const count,
        std::uint_fast16_t const max_count = 0xFFFFU) noexcept;

    //! wait (block) on the semaphore
    //!
    //! @details
    //! When an extended thread calls QXSemaphore::wait() and the value of the
    //! semaphore counter is greater than 0, QXSemaphore_wait() decrements the
    //! semaphore counter and returns (true) to its caller. However, if the
    //! value of the semaphore counter is 0, the function places the calling
    //! thread in the waiting list for the semaphore. The thread waits until
    //! the semaphore is signaled by calling QXSemaphore::signal(), or the
    //! specified timeout expires. If the semaphore is signaled before the
    //! timeout expires, QXK resumes the highest-priority extended thread
    //! waiting for the semaphore.
    //!
    //! @param[in]  nTicks    number of clock ticks (at the associated rate)
    //!                       to wait for the semaphore. The value of
    //!                       QP::QXTHREAD_NO_TIMEOUT indicates that no
    //!                       timeout will occur and the semaphore will wait
    //!                       indefinitely.
    //! @returns
    //! true if the semaphore has been signaled, and false if the timeout
    //! occurred.
    //!
    //! @note
    //! Multiple extended threads can wait for a given semaphore.
    bool wait(std::uint_fast16_t const nTicks = QXTHREAD_NO_TIMEOUT) noexcept;

    //! try wait on the semaphore (non-blocking)
    //!
    //! @details
    //! This operation checks if the semaphore counter is greater than 0,
    //! in which case the counter is decremented.
    //!
    //! @returns
    //! 'true' if the semaphore has count available and 'false' NOT available.
    //!
    //! @note
    //! This function can be called from any context, including ISRs and
    //! basic threads (active objects).
    bool tryWait() noexcept;

    //! signal (unblock) the semaphore
    //!
    //! @details
    //! If the semaphore counter value is 0 or more, it is incremented, and
    //! this function returns to its caller. If the extended threads are
    //! waiting for the semaphore to be signaled, QXSemaphore::signal()
    //! removes the highest-priority thread waiting for the semaphore from
    //! the waiting list and makes this thread ready-to-run. The QXK
    //! scheduler is then called to determine if the awakened thread is now
    //! the highest-priority thread that is ready-to-run.
    //!
    //! @returns
    //! 'true' when the semaphore gets signaled and 'false' when the
    //! semaphore count exceeded the maximum.
    //!
    //! @note
    //! A semaphore can be signaled from many places, including from ISRs,
    //! basic threads (AOs), and extended threads.
    bool signal() noexcept;
}; // class QXSemaphore

} // namespace QP
//$enddecl${QXK::QXSemaphore} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$declare${QXK::QXMutex} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QXK::QXMutex} ............................................................
//! Blocking, Priority-Ceiling Mutex the QXK preemptive kernel
//!
//! @details
//! QP::QXMutex is a blocking mutual exclusion mechanism that can also apply
//! the **priority-ceiling protocol** to avoid unbounded priority inversion
//! (if initialized with a non-zero ceiling priority, see QXMutex::init()).
//! In that case, QP::QXMutex requires its own uinque QP priority level,
//! which cannot be used by any thread or any other QP::QXMutex.
//! If initialized with preemption-ceiling of zero, QXMutex does **not**
//! use the priority-ceiling protocol and does not require a unique QP
//! priority (see QXMutex::init()).
//! QP::QXMutex is **recursive** (re-entrant), which means that it can be
//! locked multiple times (up to 255 levels) by the *same* thread without
//! causing deadlock.<br>
//!
//! QP::QXMutex is primarily intended for the @ref QP::QXThread
//! "extended (blocking) threads", but can also be used by the
//! @ref QPP::QActive "basic threads" through the non-blocking
//! QXMutex::tryLock() API.
//!
//! @note
//! QP::QXMutex should be used in situations when at least one of the extended
//! threads contending for the mutex blocks while holding the mutex (between
//! the QXMutex::lock() and QXMutex_unlock() operations). If no blocking is
//! needed while holding the mutex, the more efficient non-blocking mechanism
//! of @ref srs_qxk_schedLock() "selective QXK scheduler locking" should be
//! used instead. @ref srs_qxk_schedLock() "Selective scheduler locking" is
//! available for both @ref QP::QActive "basic threads" and @ref QP::QXThread
//! "extended threads", so it is applicable to situations where resources
//! are shared among all these threads.
//!
//! @usage
//! The following example illustrates how to instantiate and use the mutex
//! in your application.
//! @include qxk_mutex.cpp
//!
class QXMutex : public QP::QActive {
private:

    //! set of extended-threads waiting on this mutex
    QPSet m_waitSet;

public:

    //! default constructor
    QXMutex();

    //! initialize the QXK priority-ceiling mutex QP::QXMutex
    //!
    //! @details
    //! Initialize the QXK priority ceiling mutex.
    //!
    //! @param[in] prioSpec  the priority specification for the mutex
    //!                      (See also QP::QPrioSpec). This value might
    //!                      also be zero.
    //! @note
    //! `prioSpec == 0` means that the priority-ceiling protocol shall **not**
    //! be used by this mutex. Such mutex will **not** change (boost) the
    //! priority of the holding thread.
    //!
    //! @note
    //! `prioSpec == 0` means that the priority-ceiling protocol shall **not**
    //! be used by this mutex. Such mutex will **not** change (boost) the
    //! priority of the holding threads.<br>
    //!
    //! Conversely, `prioSpec != 0` means that the priority-ceiling protocol
    //! shall be used by this mutex. Such mutex **will** temporarily boost
    //! the priority and priority-threshold of the holding thread to the
    //! priority specification in `prioSpec` (see QP::QPrioSpec).
    //!
    //! @usage
    //! @include qxk_mutex.cpp
    void init(QPrioSpec const prioSpec) noexcept;

    //! try to lock the QXK priority-ceiling mutex QP::QXMutex
    //!
    //! @details
    //! Try to lock the QXK priority ceiling mutex QP::QXMutex.
    //!
    //! @returns
    //! 'true' if the mutex was successfully locked and 'false' if the mutex
    //! was unavailable and was NOT locked.
    //!
    //! @note
    //! This function **can** be called from both basic threads (active
    //! objects) and extended threads.
    //!
    //! @note
    //! The mutex locks are allowed to nest, meaning that the same extended
    //! thread can lock the same mutex multiple times (< 255). However, each
    //! successful call to QXMutex::tryLock() must be balanced by the
    //! matching call to QXMutex::unlock().
    bool tryLock() noexcept;

    //! lock the QXK priority-ceiling mutex QP::QXMutex
    //!
    //! @details
    //! Lock the QXK priority ceiling mutex QP::QXMutex.
    //!
    //! @param[in]  nTicks  number of clock ticks (at the associated rate)
    //!                     to wait for the mutex. The value of
    //!                     QXTHREAD_NO_TIMEOUT indicates that no timeout will
    //!                     occur and the mutex could block indefinitely.
    //! @returns
    //! 'true' if the mutex has been acquired and 'false' if a timeout
    //! occurred.
    //!
    //! @note
    //! The mutex locks are allowed to nest, meaning that the same extended
    //! thread can lock the same mutex multiple times (< 255). However,
    //! each call to QXMutex::lock() must be balanced by the matching call to
    //! QXMutex::unlock().
    //!
    //! @usage
    //! @include qxk_mutex.cpp
    bool lock(std::uint_fast16_t const nTicks = QXTHREAD_NO_TIMEOUT) noexcept;

    //! unlock the QXK priority-ceiling mutex QP::QXMutex
    //!
    //! @details
    //! Unlock the QXK priority ceiling mutex.
    //!
    //! @note
    //! This function **can** be called from both basic threads (active
    //! objects) and extended threads.
    //!
    //! @note
    //! The mutex locks are allowed to nest, meaning that the same extended
    //! thread can lock the same mutex multiple times (< 255). However, each
    //! call to QXMutex::lock() or a *successful* call to QXMutex::tryLock()
    //! must be balanced by the matching call to QXMutex::unlock().
    //!
    //! @usage
    //! @include qxk_mutex.cpp
    void unlock() noexcept;
}; // class QXMutex

} // namespace QP
//$enddecl${QXK::QXMutex} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//============================================================================
extern "C" {
//$declare${QXK-extern-C} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QXK-extern-C::QXK_Attr} ..................................................
//! attributes of the QXK kernel (extern "C" for easy access in assembly)
struct QXK_Attr {
    QP::QActive * volatile curr;      //!< currently executing thread
    QP::QActive * volatile next;      //!< next thread to execute
    QP::QActive * volatile prev;      //!< previous thread
    std::uint8_t volatile actPrio;    //!< QF-prio of the active AO
    std::uint8_t volatile lockCeil;   //!< lock preemption-ceiling (0==no-lock)
    std::uint8_t volatile lockHolder; //!< prio of the lock holder
};

//${QXK-extern-C::QXK_attr_} .................................................
//! attributes of the QXK kernel (extern "C" to be accessible from C)
extern QXK_Attr QXK_attr_;

//${QXK-extern-C::QXK_sched_} ................................................
//! QXK scheduler finds the highest-priority thread ready to run
//!
//! @details
//! The QXK scheduler finds the priority of the highest-priority thread
//! that is ready to run.
//!
//! @returns the 1-based priority of the the active object to run next,
//! or zero if no eligible active object is found.
//!
//! @attention
//! QXK_sched_() must be always called with interrupts **disabled** and
//! returns with interrupts **disabled**.
std::uint_fast8_t QXK_sched_() noexcept;

//${QXK-extern-C::QXK_activate_} .............................................
//! QXK activator activates the next active object. The activated AO preempts
//! the currently executing AOs
//!
//! @attention
//! QXK_activate_() must be always called with interrupts **disabled** and
//! returns with interrupts **disabled**.
//!
//! @note
//! The activate function might enable interrupts internally, but it always
//! returns with interrupts **disabled**.
void QXK_activate_() noexcept;

//${QXK-extern-C::QXK_current} ...............................................
//! obtain the currently executing active-object/thread
//!
//! @returns
//! pointer to the currently executing active-object/thread
QP::QActive * QXK_current() noexcept;

//${QXK-extern-C::QXK_stackInit_} ............................................
//! initialize the private stack of a given AO
void QXK_stackInit_(
    void * thr,
     QP::QXThreadHandler const handler,
    void * const stkSto,
    std::uint_fast16_t const stkSize) noexcept;

//${QXK-extern-C::QXK_contextSw} .............................................
#if defined(Q_SPY) || defined(QXK_ON_CONTEXT_SW)
//! QXK context switch management
//!
//! @details
//! This function performs software tracing (if #Q_SPY is defined)
//! and calls QXK_onContextSw() (if #QXK_ON_CONTEXT_SW is defined)
//!
//! @param[in] next  pointer to the next thread (NULL for basic-thread)
//!
//! @attention
//! QXK_contextSw() is invoked with interrupts **disabled** and must also
//! return with interrupts **disabled**.
void QXK_contextSw(QP::QActive * const next);
#endif //  defined(Q_SPY) || defined(QXK_ON_CONTEXT_SW)

//${QXK-extern-C::QXK_onContextSw} ...........................................
#ifdef QXK_ON_CONTEXT_SW
//! QXK context switch callback (customized in BSPs for QXK)
//!
//! @details
//! This callback function provides a mechanism to perform additional
//! custom operations when QXK switches context from one thread to
//! another.
//!
//! @param[in] prev   pointer to the previous thread (active object)
//!                   (prev==0 means that `prev` was the QXK idle thread)
//! @param[in] next   pointer to the next thread (active object)
//!                   (next==0) means that `next` is the QXK idle thread)
//! @attention
//! QXK_onContextSw() is invoked with interrupts **disabled** and must also
//! return with interrupts **disabled**.
//!
//! @note
//! This callback is enabled by defining the macro #QXK_ON_CONTEXT_SW.
//!
//! @include qxk_oncontextsw.cpp
void QXK_onContextSw(
    QP::QActive * prev,
    QP::QActive * next);
#endif // def QXK_ON_CONTEXT_SW

//${QXK-extern-C::QXK_threadExit_} ...........................................
//! called when a thread function exits
//!
//! @details
//! Called when the extended-thread handler function exits.
//!
//! @note
//! Most thread handler functions are structured as endless loops that never
//! exit. But it is also possible to structure threads as one-shot functions
//! that perform their job and exit. In that case this function peforms
//! cleanup after the thread.
void QXK_threadExit_();

//${QXK-extern-C::QXK_PTR_CAST_} .............................................
//! intertnal macro to encapsulate casting of pointers for MISRA deviations
//!
//! @details
//! This macro is specifically and exclusively used for casting pointers
//! that are never de-referenced, but only used for internal bookkeeping and
//! checking (via assertions) the correct operation of the QXK kernel.
//! Such pointer casting is not compliant with MISRA C++ Rule 5-2-7
//! as well as other messages (e.g., PC-Lint-Plus warning 826).
//! Defining this specific macro for this purpose allows to selectively
//! disable the warnings for this particular case.
#define QXK_PTR_CAST_(type_, ptr_) (reinterpret_cast<type_>(ptr_))

//${QXK-extern-C::QXTHREAD_CAST_} ............................................
//! internal macro to encapsulate casting of pointers for MISRA deviations
//!
//! @details
//! This macro is specifically and exclusively used for downcasting pointers
//! to QActive to pointers to QXThread in situations when it is known
//! that such downcasting is correct.<br>
//!
//! However, such pointer casting is not compliant with MISRA C++
//! Rule 5-2-7 as well as other messages (e.g., PC-Lint-Plus warning 826).
//! Defining this specific macro for this purpose allows to selectively
//! disable the warnings for this particular case.
#define QXTHREAD_CAST_(ptr_) (static_cast<QP::QXThread *>(ptr_))
//$enddecl${QXK-extern-C} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
} // extern "C"
//============================================================================
// interface used only inside QF, but not in applications
#ifdef QP_IMPL
// QXK implementation...
//$declare${QXK-impl} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QXK-impl::QXK_ISR_CONTEXT_} ..............................................
#ifndef QXK_ISR_CONTEXT_
//! Internal port-specific macro that checks the execution context
//! (ISR vs. thread). Might be overridden in qxk_port.hpp.
//!
//! @returns
//! 'true' if the code executes in the ISR context and 'false' otherwise.
#define QXK_ISR_CONTEXT_() (QF::intNest_ != 0U)
#endif // ndef QXK_ISR_CONTEXT_

//${QXK-impl::QF_SCHED_STAT_} ................................................
//! QXK scheduler lock status
#define QF_SCHED_STAT_ QSchedStatus lockStat_;

//${QXK-impl::QF_SCHED_LOCK_} ................................................
//! QXK selective scheduler locking
#define QF_SCHED_LOCK_(ceil_) do { \
    if (QXK_ISR_CONTEXT_()) { \
        lockStat_ = 0xFFU; \
    } else { \
        lockStat_ = QXK::schedLock((ceil_)); \
    } \
} while (false)

//${QXK-impl::QF_SCHED_UNLOCK_} ..............................................
//! QXK selective scheduler unlocking
#define QF_SCHED_UNLOCK_() do { \
    if (lockStat_ != 0xFFU) { \
        QXK::schedUnlock(lockStat_); \
    } \
} while (false)

//${QXK-impl::QACTIVE_EQUEUE_WAIT_} ..........................................
// QXK native event queue waiting
#define QACTIVE_EQUEUE_WAIT_(me_) \
   Q_ASSERT_ID(110, (me_)->m_eQueue.m_frontEvt != nullptr)

//${QXK-impl::QACTIVE_EQUEUE_SIGNAL_} ........................................
// QXK native event queue signalling
#define QACTIVE_EQUEUE_SIGNAL_(me_) do { \
    QF::readySet_.insert( \
        static_cast<std::uint_fast8_t>((me_)->m_prio)); \
    if (!QXK_ISR_CONTEXT_()) { \
        if (QXK_sched_() != 0U) { \
            QXK_activate_(); \
        } \
    } \
} while (false)
//$enddecl${QXK-impl} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// Native QF event pool operations...
//$declare${QF-QMPool-impl} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QF-QMPool-impl::QF_EPOOL_TYPE_} ..........................................
#define QF_EPOOL_TYPE_ QMPool

//${QF-QMPool-impl::QF_EPOOL_INIT_} ..........................................
#define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
    (p_).init((poolSto_), (poolSize_), (evtSize_))

//${QF-QMPool-impl::QF_EPOOL_EVENT_SIZE_} ....................................
#define QF_EPOOL_EVENT_SIZE_(p_) ((p_).getBlockSize())

//${QF-QMPool-impl::QF_EPOOL_GET_} ...........................................
#define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
    ((e_) = static_cast<QEvt *>((p_).get((m_), (qs_id_))))

//${QF-QMPool-impl::QF_EPOOL_PUT_} ...........................................
#define QF_EPOOL_PUT_(p_, e_, qs_id_) ((p_).put((e_), (qs_id_)))
//$enddecl${QF-QMPool-impl} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif // QP_IMPL

#endif // QP_INC_QXK_HPP_
