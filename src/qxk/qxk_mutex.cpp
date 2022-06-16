//============================================================================
// QP/C++ Real-Time Embedded Framework (RTEF)
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
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
// <www.state-machine.com>
// <info@state-machine.com>
//============================================================================
//! @date Last updated on: 2022-06-15
//! @version Last updated for: @ref qpcpp_7_0_1
//!
//! @file
//! @brief Priority-ceiling blocking mutex QP::QXMutex class definition

#define QP_IMPL             // this is QP implementation
#include "qf_port.hpp"      // QF port
#include "qxk_pkg.hpp"      // QXK package-scope interface
#include "qassert.h"        // QP embedded systems-friendly assertions
#ifdef Q_SPY                // QS software tracing enabled?
    #include "qs_port.hpp"  // QS port
    #include "qs_pkg.hpp"   // QS facilities for pre-defined trace records
#else
    #include "qs_dummy.hpp" // disable the QS software tracing
#endif // Q_SPY

// protection against including this source file in a wrong project
#ifndef QXK_HPP
    #error "Source file included in a project NOT based on the QXK kernel"
#endif // QXK_HPP

//============================================================================
namespace { // unnamed local namespace

Q_DEFINE_THIS_MODULE("qxk_mutex")

} // unnamed namespace

//============================================================================
namespace QP {


//............................................................................
void QXMutex::init(std::uint_fast8_t const ceiling) noexcept {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    //! @pre the celiling priority of the mutex must:
    //! - cannot exceed the maximum #QF_MAX_ACTIVE;
    //! - the ceiling priority of the mutex must not be already in use;
    //! (QF requires priority to be **unique**).
    Q_REQUIRE_ID(100,
        (ceiling <= QF_MAX_ACTIVE)
        && ((ceiling == 0U)
            || (QF::active_[ceiling] == nullptr)));

    m_ceiling    = static_cast<std::uint8_t>(ceiling);
    m_lockNest   = 0U;
    m_holderPrio = 0U;
    QF::bzero(&m_waitSet, sizeof(m_waitSet));

    if (ceiling != 0U) {
        // reserve the ceiling priority level for this mutex
        QF::active_[ceiling] = QXK_PTR_CAST_(QActive*, this);
    }
    QF_CRIT_X_();
}

//............................................................................
bool QXMutex::lock(std::uint_fast16_t const nTicks) noexcept {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QXThread * const curr = QXK_PTR_CAST_(QXThread*, QXK_attr_.curr);

    //! @pre this function must:
    //! - NOT be called from an ISR;
    //! - be called from an extended thread;
    //! - the ceiling priority must not be used; or if used
    //!   - the thread priority must be below the ceiling of the mutex;
    //! - the ceiling must be in range
    //! - the thread must NOT be already blocked on any object.
    //!
    Q_REQUIRE_ID(200, (!QXK_ISR_CONTEXT_())
        && (curr != nullptr)
        && ((m_ceiling == 0U)
            || (curr->m_prio < m_ceiling))
        && (m_ceiling <= QF_MAX_ACTIVE)
        && (curr->m_temp.obj == nullptr)); // not blocked
    //! @pre also: the thread must NOT be holding a scheduler lock.
    Q_REQUIRE_ID(201, QXK_attr_.lockHolder != curr->m_prio);

    // is the mutex available?
    bool locked = true; // assume that the mutex will be locked
    if (m_lockNest == 0U) {
        m_lockNest = 1U;

        if (m_ceiling != 0U) {
            // the priority slot must be occupied by this mutex
            Q_ASSERT_ID(210, QF::active_[m_ceiling]
                             == QXK_PTR_CAST_(QActive*, this));

            // boost the dynamic priority of this thread to the ceiling
            QXK_attr_.readySet.rmove(
                static_cast<std::uint_fast8_t>(curr->m_dynPrio));
            curr->m_dynPrio = m_ceiling;
            QXK_attr_.readySet.insert(
                static_cast<std::uint_fast8_t>(curr->m_dynPrio));
            QF::active_[m_ceiling] = curr;
        }

        // make the curr thread the new mutex holder
        m_holderPrio = static_cast<std::uint8_t>(curr->m_prio);

        QS_BEGIN_NOCRIT_PRE_(QS_MUTEX_LOCK, curr->m_prio)
            QS_TIME_PRE_();  // timestamp
            // start prio & current ceiling
            QS_2U8_PRE_(curr->m_prio, m_ceiling);
        QS_END_NOCRIT_PRE_()
    }
    // is the mutex locked by this thread already (nested locking)?
    else if (m_holderPrio == curr->m_prio) {

        // the nesting level must not exceed the dynamic range of uint8_t
        Q_ASSERT_ID(220, m_lockNest < 0xFFU);

        m_lockNest = (m_lockNest + 1U); // lock one level
    }
    else { // the mutex is alredy locked by a different thread

        // the ceiling holder priority must be valid
        Q_ASSERT_ID(230, 0U < m_holderPrio);
        Q_ASSERT_ID(231, m_holderPrio <= QF_MAX_ACTIVE);

        if (m_ceiling != 0U) {
            // the prio slot must be occupied by the thr. holding the mutex
            Q_ASSERT_ID(240, QF::active_[m_ceiling]
                             == QF::active_[m_holderPrio]);
        }

        // remove the curr dynamic prio from the ready set (block)
        // and insert it to the waiting set on this mutex
        std::uint_fast8_t const p =
            static_cast<std::uint_fast8_t>(curr->m_dynPrio);
        QXK_attr_.readySet.rmove(p);
        m_waitSet.insert(p);

        // store the blocking object (this mutex)
        curr->m_temp.obj = QXK_PTR_CAST_(QMState*, this);
        curr->teArm_(static_cast<enum_t>(QXK_SEMA_SIG), nTicks);

        // schedule the next thread if multitasking started
        static_cast<void>(QXK_sched_());
        QF_CRIT_X_();
        QF_CRIT_EXIT_NOP(); // BLOCK here !!!

        QF_CRIT_E_();   // AFTER unblocking...
        // the blocking object must be this mutex
        Q_ASSERT_ID(240, curr->m_temp.obj == QXK_PTR_CAST_(QMState *, this));

        // did the blocking time-out? (signal of zero means that it did)
        if (curr->m_timeEvt.sig == 0U) {
            if (m_waitSet.hasElement(p)) { // still waiting?
                m_waitSet.rmove(p); // remove the unblocked thread
                locked = false; // the mutex was NOT locked
            }
        }
        else { // blocking did NOT time out
            // the thread must NOT be waiting on this mutex
            Q_ASSERT_ID(250, !m_waitSet.hasElement(p));
        }
        curr->m_temp.obj = nullptr; // clear blocking obj.
    }
    QF_CRIT_X_();

    return locked;
}

//............................................................................
bool QXMutex::tryLock(void) noexcept {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QActive *curr = QXK_attr_.curr;
    if (curr == nullptr) { // called from a basic thread?
        curr = QF::active_[QXK_attr_.actPrio];
    }

    //! @pre this function must:
    //! - NOT be called from an ISR;
    //! - the calling thread must be valid;
    //! - the ceiling must be not used; or
    //!   - the thread priority must be below the ceiling of the mutex;
    //! - the ceiling must be in range
    Q_REQUIRE_ID(300, (!QXK_ISR_CONTEXT_())
        && (curr != nullptr)
        && ((m_ceiling == 0U)
            || (curr->m_prio < m_ceiling))
        && (m_ceiling <= QF_MAX_ACTIVE));
    //! @pre also: the thread must NOT be holding a scheduler lock.
    Q_REQUIRE_ID(301, QXK_attr_.lockHolder != curr->m_prio);

    // is the mutex available?
    if (m_lockNest == 0U) {
        m_lockNest = 1U;

        if (m_ceiling != 0U) {
            // the priority slot must be set to this mutex
            Q_ASSERT_ID(310, QF::active_[m_ceiling]
                             == QXK_PTR_CAST_(QActive *, this));

            // boost the dynamic priority of this thread to the ceiling
            QXK_attr_.readySet.rmove(
                static_cast<std::uint_fast8_t>(curr->m_dynPrio));
            curr->m_dynPrio = m_ceiling;
            QXK_attr_.readySet.insert(
                static_cast<std::uint_fast8_t>(curr->m_dynPrio));
            QF::active_[m_ceiling] = curr;
        }

        // make curr thread the new mutex holder
        m_holderPrio = curr->m_prio;

        QS_BEGIN_NOCRIT_PRE_(QS_MUTEX_LOCK, curr->m_prio)
            QS_TIME_PRE_();  // timestamp
            // start prio & current ceiling
            QS_2U8_PRE_(curr->m_prio, m_ceiling);
        QS_END_NOCRIT_PRE_()
    }
    // is the mutex held by this thread already (nested locking)?
    else if (m_holderPrio == curr->m_prio) {
        // the nesting level must not exceed  the dynamic range of uint8_t
        Q_ASSERT_ID(320, m_lockNest < 0xFFU);

        m_lockNest = (m_lockNest + 1U); // lock one level
    }
    else { // the mutex is alredy locked by a different thread
        if (m_ceiling != 0U) {
            // the prio slot must be claimed by the mutex holder
            Q_ASSERT_ID(330, QF::active_[m_ceiling]
                             == QF::active_[m_holderPrio]);
        }
        curr = nullptr; // means that mutex is NOT available
    }
    QF_CRIT_X_();

    return curr != nullptr;
}

//............................................................................
void QXMutex::unlock(void) noexcept {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QActive *curr = static_cast<QActive *>(QXK_attr_.curr);
    if (curr == nullptr) { // called from a basic thread?
        curr = QF::active_[QXK_attr_.actPrio];
    }

    //! @pre this function must:
    //! - NOT be called from an ISR;
    //! - the calling thread must be valid;
    //! - the ceiling must not be used or
    //!    - the current thread must have priority equal to the mutex ceiling;
    //! - the ceiling must be in range
    //!
    Q_REQUIRE_ID(400, (!QXK_ISR_CONTEXT_())
        && (curr != nullptr)
        && ((m_ceiling == 0U)
            || (curr->m_dynPrio == m_ceiling))
        && (m_ceiling <= QF_MAX_ACTIVE));
    //! @pre also: the mutex must be already locked at least once.
    Q_REQUIRE_ID(401, m_lockNest > 0U);
    //! @pre also: the mutex must be held by this thread.
    Q_REQUIRE_ID(402, m_holderPrio == curr->m_prio);

    // is this the last nesting level?
    if (m_lockNest == 1U) {

        if (m_ceiling != 0U) {
            // restore the holding thread's priority to the original
            QXK_attr_.readySet.rmove(
                static_cast<std::uint_fast8_t>(curr->m_dynPrio));
            curr->m_dynPrio = curr->m_prio;
            QXK_attr_.readySet.insert(
                static_cast<std::uint_fast8_t>(curr->m_dynPrio));

            // put the mutex at the priority ceiling slot
            QF::active_[m_ceiling] = QXK_PTR_CAST_(QActive*, this);
        }

        // the mutex no longer held by a thread
        m_holderPrio = 0U;

        QS_BEGIN_NOCRIT_PRE_(QS_MUTEX_UNLOCK, curr->m_prio)
            QS_TIME_PRE_();  // timestamp
            // start prio & the mutex ceiling
            QS_2U8_PRE_(curr->m_prio, m_ceiling);
        QS_END_NOCRIT_PRE_()

        // are any other threads waiting for this mutex?
        if (m_waitSet.notEmpty()) {

            // find the highest-priority thread waiting on this mutex
            std::uint_fast8_t const p = m_waitSet.findMax();
            QXThread * const thr = QXK_PTR_CAST_(QXThread*, QF::active_[p]);

            // the waiting thread must:
            // - the ceiling must not be used; or if used
            //   - the thread must have priority below the ceiling
            // - be registered in QF
            // - have the dynamic priority the same as initial priority
            // - be blocked on this mutex
            Q_ASSERT_ID(410,
                ((m_ceiling == 0U)
                   || (p < static_cast<std::uint_fast8_t>(m_ceiling)))
                && (thr != nullptr)
                && (thr->m_dynPrio == thr->m_prio)
                && (thr->m_temp.obj == QXK_PTR_CAST_(QMState*, this)));

            // disarm the internal time event
            static_cast<void>(thr->teDisarm_());

            if (m_ceiling != 0U) {
                // boost the dynamic priority of this thread to the ceiling
                thr->m_dynPrio = m_ceiling;
                QF::active_[m_ceiling] = thr;
            }

            // make the thread the new mutex holder
            m_holderPrio = static_cast<std::uint8_t>(thr->m_prio);

            // make the thread ready to run (at the ceiling prio)
            // and remove from the waiting list
            QXK_attr_.readySet.insert(thr->m_dynPrio);
            m_waitSet.rmove(p);

            QS_BEGIN_NOCRIT_PRE_(QS_MUTEX_LOCK, thr->m_prio)
                QS_TIME_PRE_();  // timestamp
                // start priority & ceiling priority
                QS_2U8_PRE_(thr->m_prio, m_ceiling);
            QS_END_NOCRIT_PRE_()
        }
        else { // no threads are waiting for this mutex
            m_lockNest = 0U;

            if (m_ceiling != 0U) {
                // put the mutex at the priority ceiling slot
                QF::active_[m_ceiling] = QXK_PTR_CAST_(QActive*, this);
            }
        }

        // schedule the next thread if multitasking started
        if (QXK_sched_() != 0U) {
            QXK_activate_(); // activate a basic thread
        }
    }
    else { // releasing the mutex
        m_lockNest = (m_lockNest - 1U); // unlock one level
    }
    QF_CRIT_X_();
}

} // namespace QP

