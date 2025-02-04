//$file${src::qxk::qxk_mutex.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qxk::qxk_mutex.cpp}
//
// This code has been generated by QM 5.3.0 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #    : LicenseRef-QL-dual
// Issued to    : Any user of the QP/C++ real-time embedded framework
// Framework(s) : qpcpp
// Support ends : 2024-12-31
// License scope:
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
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
//$endhead${src::qxk::qxk_mutex.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define QP_IMPL             // this is QP implementation
#include "qp_port.hpp"      // QP port
#include "qp_pkg.hpp"       // QP package-scope interface
#include "qsafe.h"          // QP Functional Safety (FuSa) Subsystem
#ifdef Q_SPY                // QS software tracing enabled?
    #include "qs_port.hpp"  // QS port
    #include "qs_pkg.hpp"   // QS facilities for pre-defined trace records
#else
    #include "qs_dummy.hpp" // disable the QS software tracing
#endif // Q_SPY

// protection against including this source file in a wrong project
#ifndef QXK_HPP_
    #error "Source file included in a project NOT based on the QXK kernel"
#endif // QXK_HPP_

// unnamed namespace for local definitions with internal linkage
namespace {
Q_DEFINE_THIS_MODULE("qxk_mutex")
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QXK::QXMutex} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QXK::QXMutex} ............................................................

//${QXK::QXMutex::QXMutex} ...................................................
QXMutex::QXMutex()
  : m_ao(Q_STATE_CAST(0))
{}

//${QXK::QXMutex::init} ......................................................
void QXMutex::init(QPrioSpec const prioSpec) noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(100, (prioSpec & 0xFF00U) == 0U);

    m_ao.m_prio  = static_cast<std::uint8_t>(prioSpec & 0xFFU); // QF-prio.
    m_ao.m_pthre = 0U; // not used
    QActive &ao  = m_ao;

    QF_MEM_APP();
    QF_CRIT_EXIT();

    ao.register_(); // register this mutex as AO
}

//${QXK::QXMutex::lock} ......................................................
bool QXMutex::lock(QTimeEvtCtr const nTicks) noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    QXThread * const curr = QXK_PTR_CAST_(QXThread*, QXK_priv_.curr);

    // precondition, this mutex operation must:
    // - NOT be called from an ISR;
    // - be called from an eXtended thread;
    // - the mutex-prio. must be in range;
    // - the thread must NOT be already blocked on any object.
    Q_REQUIRE_INCRIT(200, (!QXK_ISR_CONTEXT_())
        && (curr != nullptr)
        && (m_ao.m_prio <= QF_MAX_ACTIVE)
        && (curr->m_temp.obj == nullptr));
    // also: the thread must NOT be holding a scheduler lock.
    Q_REQUIRE_INCRIT(201, QXK_priv_.lockHolder != curr->m_prio);

    // is the mutex available?
    bool locked = true; // assume that the mutex will be locked
    if (m_ao.m_eQueue.m_nFree == 0U) {
        m_ao.m_eQueue.m_nFree = 1U; // mutex lock nesting

        // also: the newly locked mutex must have no holder yet
        Q_REQUIRE_INCRIT(203, m_ao.m_osObject == nullptr);

        // set the new mutex holder to the curr thread and
        // save the thread's prio in the mutex
        // NOTE: reuse the otherwise unused eQueue data member.
        m_ao.m_osObject = curr;
        m_ao.m_eQueue.m_head = static_cast<QEQueueCtr>(curr->m_prio);

        QS_BEGIN_PRE_(QS_MTX_LOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head));
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_nFree));
        QS_END_PRE_()

        if (m_ao.m_prio != 0U) { // prio.-ceiling protocol used?
            // the holder prio. must be lower than that of the mutex
            // and the prio. slot must be occupied by this mutex
            Q_ASSERT_INCRIT(210, (curr->m_prio < m_ao.m_prio)
                && (QActive::registry_[m_ao.m_prio] == &m_ao));

            // remove the thread's original prio from the ready set
            // and insert the mutex's prio into the ready set
            QXK_priv_.readySet.remove(
                static_cast<std::uint_fast8_t>(m_ao.m_eQueue.m_head));
            QXK_priv_.readySet.insert(
                static_cast<std::uint_fast8_t>(m_ao.m_prio));
    #ifndef Q_UNSAFE
            QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif
            // put the thread into the AO registry in place of the mutex
            QActive::registry_[m_ao.m_prio] = curr;

            // set thread's prio to that of the mutex
            curr->m_prio  = m_ao.m_prio;
    #ifndef Q_UNSAFE
            curr->m_prio_dis = static_cast<std::uint8_t>(~curr->m_prio);
    #endif
        }
    }
    // is the mutex locked by this thread already (nested locking)?
    else if (m_ao.m_osObject == curr) {

        // the nesting level beyond the arbitrary but high limit
        // most likely means cyclic or recursive locking of a mutex.
        Q_ASSERT_INCRIT(220, m_ao.m_eQueue.m_nFree < 0xFFU);

        // lock one more level
        m_ao.m_eQueue.m_nFree = m_ao.m_eQueue.m_nFree + 1U;

        QS_BEGIN_PRE_(QS_MTX_LOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head));
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_nFree));
        QS_END_PRE_()
    }
    else { // the mutex is already locked by a different thread
        // the mutex holder must be valid
        Q_ASSERT_INCRIT(230, m_ao.m_osObject != nullptr);

        if (m_ao.m_prio != 0U) { // prio.-ceiling protocol used?
            // the prio slot must be occupied by the thr. holding the mutex
            Q_ASSERT_INCRIT(240, QActive::registry_[m_ao.m_prio]
                == QXK_PTR_CAST_(QActive const *, m_ao.m_osObject));
        }

        // remove the curr thread's prio from the ready set (will block)
        // and insert it to the waiting set on this mutex
        std::uint_fast8_t const p =
            static_cast<std::uint_fast8_t>(curr->m_prio);
        QXK_priv_.readySet.remove(p);
    #ifndef Q_UNSAFE
        QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif
        m_waitSet.insert(p);

        // set the blocking object (this mutex)
        curr->m_temp.obj = QXK_PTR_CAST_(QMState*, this);
        curr->teArm_(static_cast<enum_t>(QXK::TIMEOUT_SIG), nTicks);

        QS_BEGIN_PRE_(QS_MTX_BLOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_2U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head),
                        curr->m_prio);
        QS_END_PRE_()

        // schedule the next thread if multitasking started
        static_cast<void>(QXK_sched_()); // schedule other threads

        QF_MEM_APP();
        QF_CRIT_EXIT();
        QF_CRIT_EXIT_NOP(); // BLOCK here !!!

        // AFTER unblocking...
        QF_CRIT_ENTRY();
        QF_MEM_SYS();
        // the blocking object must be this mutex
        Q_ASSERT_INCRIT(250, curr->m_temp.obj
                         == QXK_PTR_CAST_(QMState*, this));

        // did the blocking time-out? (signal of zero means that it did)
        if (curr->m_timeEvt.sig == 0U) {
            if (m_waitSet.hasElement(p)) { // still waiting?
                m_waitSet.remove(p); // remove unblocked thread
                locked = false; // the mutex was NOT locked
            }
        }
        else { // blocking did NOT time out
            // the thread must NOT be waiting on this mutex
            Q_ASSERT_INCRIT(260, !m_waitSet.hasElement(p));
        }
        curr->m_temp.obj = nullptr; // clear blocking obj.
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();

    return locked;
}

//${QXK::QXMutex::tryLock} ...................................................
bool QXMutex::tryLock() noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    QActive *curr = QXK_priv_.curr;
    if (curr == nullptr) { // called from a basic thread?
        curr = QActive::registry_[QXK_priv_.actPrio];
    }

    // precondition, this mutex must:
    // - NOT be called from an ISR;
    // - the calling thread must be valid;
    // - the mutex-prio. must be in range
    Q_REQUIRE_INCRIT(300, (!QXK_ISR_CONTEXT_())
        && (curr != nullptr)
        && (m_ao.m_prio <= QF_MAX_ACTIVE));
    // also: the thread must NOT be holding a scheduler lock.
    Q_REQUIRE_INCRIT(301,
        QXK_priv_.lockHolder != static_cast<std::uint_fast8_t>(curr->m_prio));

    // is the mutex available?
    if (m_ao.m_eQueue.m_nFree == 0U) {
        m_ao.m_eQueue.m_nFree = 1U;  // mutex lock nesting

        // also the newly locked mutex must have no holder yet
        Q_REQUIRE_INCRIT(303, m_ao.m_osObject == nullptr);

        // set the new mutex holder to the curr thread and
        // save the thread's prio in the mutex
        // NOTE: reuse the otherwise unused eQueue data member.
        m_ao.m_osObject = curr;
        m_ao.m_eQueue.m_head = static_cast<QEQueueCtr>(curr->m_prio);

        QS_BEGIN_PRE_(QS_MTX_LOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head));
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_nFree));
        QS_END_PRE_()

        if (m_ao.m_prio != 0U) { // prio.-ceiling protocol used?
            // the holder prio. must be lower than that of the mutex
            // and the prio. slot must be occupied by this mutex
            Q_ASSERT_INCRIT(310, (curr->m_prio < m_ao.m_prio)
                && (QActive::registry_[m_ao.m_prio] == &m_ao));

            // remove the thread's original prio from the ready set
            // and insert the mutex's prio into the ready set
            QXK_priv_.readySet.remove(
                static_cast<std::uint_fast8_t>(m_ao.m_eQueue.m_head));
            QXK_priv_.readySet.insert(
                static_cast<std::uint_fast8_t>(m_ao.m_prio));
    #ifndef Q_UNSAFE
            QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif
            // put the thread into the AO registry in place of the mutex
            QActive::registry_[m_ao.m_prio] = curr;

            // set thread's prio to that of the mutex
            curr->m_prio  = m_ao.m_prio;
    #ifndef Q_UNSAFE
            curr->m_prio_dis = static_cast<std::uint8_t>(~curr->m_prio);
    #endif
        }
    }
    // is the mutex locked by this thread already (nested locking)?
    else if (m_ao.m_osObject == curr) {
        // the nesting level must not exceed the specified limit
        Q_ASSERT_INCRIT(320, m_ao.m_eQueue.m_nFree < 0xFFU);

        // lock one more level
        m_ao.m_eQueue.m_nFree = m_ao.m_eQueue.m_nFree + 1U;

        QS_BEGIN_PRE_(QS_MTX_LOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head));
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_nFree));
        QS_END_PRE_()
    }
    else { // the mutex is already locked by a different thread
        if (m_ao.m_prio != 0U) {  // prio.-ceiling protocol used?
            // the prio slot must be occupied by the thr. holding the mutex
            Q_ASSERT_INCRIT(330, QActive::registry_[m_ao.m_prio]
                == QXK_PTR_CAST_(QActive const *, m_ao.m_osObject));
        }

        QS_BEGIN_PRE_(QS_MTX_BLOCK_ATTEMPT, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_2U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head),
                        curr->m_prio); // trying thread prio
        QS_END_PRE_()

        curr = nullptr; // means that mutex is NOT available
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();

    return curr != nullptr;
}

//${QXK::QXMutex::unlock} ....................................................
void QXMutex::unlock() noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    QActive *curr = QXK_priv_.curr;
    if (curr == nullptr) { // called from a basic thread?
        curr = QActive::registry_[QXK_priv_.actPrio];
    }

    Q_REQUIRE_INCRIT(400, (!QXK_ISR_CONTEXT_())
        && (curr != nullptr));
    Q_REQUIRE_INCRIT(401, m_ao.m_eQueue.m_nFree > 0U);
    Q_REQUIRE_INCRIT(403, m_ao.m_osObject == curr);

    // is this the last nesting level?
    if (m_ao.m_eQueue.m_nFree == 1U) {

        if (m_ao.m_prio != 0U) { // prio.-ceiling protocol used?

            Q_ASSERT_INCRIT(410, m_ao.m_prio < QF_MAX_ACTIVE);

            // restore the holding thread's prio from the mutex
            curr->m_prio  =
                static_cast<std::uint8_t>(m_ao.m_eQueue.m_head);
    #ifndef Q_UNSAFE
            curr->m_prio_dis = static_cast<std::uint8_t>(~curr->m_prio);
    #endif

            // put the mutex back into the AO registry
            QActive::registry_[m_ao.m_prio] = &m_ao;

            // remove the mutex' prio from the ready set
            // and insert the original thread's prio.
            QXK_priv_.readySet.remove(
                static_cast<std::uint_fast8_t>(m_ao.m_prio));
            QXK_priv_.readySet.insert(
                static_cast<std::uint_fast8_t>(m_ao.m_eQueue.m_head));
    #ifndef Q_UNSAFE
            QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif
        }

        QS_BEGIN_PRE_(QS_MTX_UNLOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_2U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head),
                        0U);
        QS_END_PRE_()

        // are any other threads waiting on this mutex?
        if (m_waitSet.notEmpty()) {
            // find the highest-prio. thread waiting on this mutex
            std::uint_fast8_t const p = m_waitSet.findMax();

            // remove this thread from waiting on the mutex
            // and insert it into the ready set.
            m_waitSet.remove(p);
            QXK_priv_.readySet.insert(p);
    #ifndef Q_UNSAFE
            QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif

            QXThread * const thr =
                QXK_PTR_CAST_(QXThread*, QActive::registry_[p]);

            // the waiting thread must:
            // - be registered in QF
            // - have the prio. corresponding to the registration
            // - be an extended thread
            // - be blocked on this mutex
            Q_ASSERT_INCRIT(420, (thr != nullptr)
                && (thr->m_prio == static_cast<std::uint8_t>(p))
                && (thr->m_state.act == Q_ACTION_CAST(0))
                && (thr->m_temp.obj == QXK_PTR_CAST_(QMState*, this)));

            // disarm the internal time event
            static_cast<void>(thr->teDisarm_());

            // set the new mutex holder to the curr thread and
            // save the thread's prio in the mutex
            // NOTE: reuse the otherwise unused eQueue data member.
            m_ao.m_osObject = thr;
            m_ao.m_eQueue.m_head = static_cast<QEQueueCtr>(thr->m_prio);

            QS_BEGIN_PRE_(QS_MTX_LOCK, thr->m_prio)
                QS_TIME_PRE_();    // timestamp
                QS_OBJ_PRE_(this); // this mutex
                QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head));
                QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_nFree));
            QS_END_PRE_()

            if (m_ao.m_prio != 0U) { // prio.-ceiling protocol used?
                // the holder prio. must be lower than that of the mutex
                Q_ASSERT_INCRIT(430, (m_ao.m_prio < QF_MAX_ACTIVE)
                                     && (thr->m_prio < m_ao.m_prio));

                // put the thread into AO registry in place of the mutex
                QActive::registry_[m_ao.m_prio] = thr;
            }
        }
        else { // no threads are waiting for this mutex
            m_ao.m_eQueue.m_nFree = 0U; // free up the nesting count

            // the mutex no longer held by any thread
            m_ao.m_osObject = nullptr;
            m_ao.m_eQueue.m_head = 0U;
            m_ao.m_eQueue.m_tail = 0U;

            if (m_ao.m_prio != 0U) { // prio.-ceiling protocol used?
                // the AO priority must be in range
                Q_ASSERT_INCRIT(440, m_ao.m_prio < QF_MAX_ACTIVE);

                // put the mutex back at the original mutex slot
                QActive::registry_[m_ao.m_prio] = &m_ao;
            }
        }

        // schedule the next thread if multitasking started
        if (QXK_sched_() != 0U) { // activation needed?
            QXK_activate_(); // synchronously activate basic-thred(s)
        }
    }
    else { // releasing one level of nested mutex lock
        // unlock one level
        m_ao.m_eQueue.m_nFree = m_ao.m_eQueue.m_nFree - 1U;

        QS_BEGIN_PRE_(QS_MTX_UNLOCK_ATTEMPT, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this mutex
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_head));
            QS_U8_PRE_(static_cast<std::uint8_t>(m_ao.m_eQueue.m_nFree));
        QS_END_PRE_()
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();
}

} // namespace QP
//$enddef${QXK::QXMutex} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
