//$file${src::qxk::qxk_sema.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qxk::qxk_sema.cpp}
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
//$endhead${src::qxk::qxk_sema.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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
Q_DEFINE_THIS_MODULE("qxk_sema")
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QXK::QXSemaphore} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QXK::QXSemaphore} ........................................................

//${QXK::QXSemaphore::init} ..................................................
void QXSemaphore::init(
    std::uint_fast8_t const count,
    std::uint_fast8_t const max_count) noexcept
{
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(100, (count <= max_count)
        && (0U < max_count) && (max_count <= 0xFFU));

    m_count     = static_cast<std::uint8_t>(count);
    m_max_count = static_cast<std::uint8_t>(max_count);
    m_waitSet.setEmpty();

    QF_MEM_APP();
    QF_CRIT_EXIT();
}

//${QXK::QXSemaphore::wait} ..................................................
bool QXSemaphore::wait(QTimeEvtCtr const nTicks) noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    QXThread * const curr = QXK_PTR_CAST_(QXThread*, QXK_priv_.curr);

    // precondition, this function:
    // - must NOT be called from an ISR;
    // - the semaphore must be initialized
    // - be called from an extended thread;
    // - the thread must NOT be already blocked on any object.
    Q_REQUIRE_INCRIT(200, (!QXK_ISR_CONTEXT_())
        && (m_max_count > 0U)
        && (curr != nullptr)
        && (curr->m_temp.obj == nullptr));
    // - the thread must NOT be holding a scheduler lock.
    Q_REQUIRE_INCRIT(201, QXK_priv_.lockHolder != curr->m_prio);

    bool taken = true; // assume that the semaphore will be signaled
    if (m_count > 0U) {
        m_count = m_count - 1U; // semaphore taken: decrement the count

        QS_BEGIN_PRE_(QS_SEM_TAKE, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this semaphore
            QS_2U8_PRE_(curr->m_prio, m_count);
        QS_END_PRE_()
    }
    else { // semaphore not available -- BLOCK the thread
        std::uint_fast8_t const p =
            static_cast<std::uint_fast8_t>(curr->m_prio);
        // remove the curr prio from the ready set (will block)
        // and insert to the waiting set on this semaphore
        QXK_priv_.readySet.remove(p);
    #ifndef Q_UNSAFE
        QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif
        m_waitSet.insert(p);

        // remember the blocking object (this semaphore)
        curr->m_temp.obj = QXK_PTR_CAST_(QMState*, this);
        curr->teArm_(static_cast<enum_t>(QXK::TIMEOUT_SIG), nTicks);

        QS_BEGIN_PRE_(QS_SEM_BLOCK, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this semaphore
            QS_2U8_PRE_(curr->m_prio, m_count);
        QS_END_PRE_()

        // schedule the next thread if multitasking started
        static_cast<void>(QXK_sched_()); // schedule other threads

        QF_MEM_APP();
        QF_CRIT_EXIT();
        QF_CRIT_EXIT_NOP(); // BLOCK here !!!

        QF_CRIT_ENTRY();   // AFTER unblocking...
        QF_MEM_SYS();

        // the blocking object must be this semaphore
        Q_ASSERT_INCRIT(240, curr->m_temp.obj
                         == QXK_PTR_CAST_(QMState*, this));

        // did the blocking time-out? (signal of zero means that it did)
        if (curr->m_timeEvt.sig == 0U) {
            if (m_waitSet.hasElement(p)) { // still waiting?
                m_waitSet.remove(p); // remove unblocked thread
                taken = false; // the semaphore was NOT taken
            }
        }
        else { // blocking did NOT time out
            // the thread must NOT be waiting on this semaphore
            Q_ASSERT_INCRIT(250, !m_waitSet.hasElement(p));
        }
        curr->m_temp.obj = nullptr; // clear blocking obj.
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();

    return taken;
}

//${QXK::QXSemaphore::tryWait} ...............................................
bool QXSemaphore::tryWait() noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    // precondition:
    // - the semaphore must be initialized
    Q_REQUIRE_INCRIT(300, m_max_count > 0U);

    #ifdef Q_SPY
    QActive const * const curr = QXK_PTR_CAST_(QActive*, QXK_priv_.curr);
    #endif // Q_SPY

    bool taken;
    // is the semaphore available?
    if (m_count > 0U) {
        m_count = m_count - 1U; // semaphore signaled: decrement
        taken = true;

        QS_BEGIN_PRE_(QS_SEM_TAKE, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this semaphore
            QS_2U8_PRE_(curr->m_prio, m_count);
        QS_END_PRE_()
    }
    else { // the semaphore is NOT available (would block)
        taken = false;

        QS_BEGIN_PRE_(QS_SEM_BLOCK_ATTEMPT, curr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this semaphore
            QS_2U8_PRE_(curr->m_prio, m_count);
        QS_END_PRE_()
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();

    return taken;
}

//${QXK::QXSemaphore::signal} ................................................
bool QXSemaphore::signal() noexcept {
    bool signaled = true; // assume that the semaphore will be signaled

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    // precondition:
    // - the semaphore must be initialized
    Q_REQUIRE_INCRIT(400, m_max_count > 0U);

    // any threads blocked on this semaphore?
    if (m_waitSet.notEmpty()) {
        // find the highest-prio. thread waiting on this semaphore
        std::uint_fast8_t const p = m_waitSet.findMax();
        QXThread * const thr =
            QXK_PTR_CAST_(QXThread*, QActive::registry_[p]);

        // assert that the tread:
        // - must be registered in QF;
        // - must be extended; and
        // - must be blocked on this semaphore;
        Q_ASSERT_INCRIT(410, (thr != nullptr)
            && (thr->m_osObject != nullptr)
            && (thr->m_temp.obj
                == QXK_PTR_CAST_(QMState*, this)));

        // disarm the internal time event
        static_cast<void>(thr->teDisarm_());

        // make the thread ready to run and remove from the wait-list
        QXK_priv_.readySet.insert(p);
    #ifndef Q_UNSAFE
        QXK_priv_.readySet.update_(&QXK_priv_.readySet_dis);
    #endif
        m_waitSet.remove(p);

        QS_BEGIN_PRE_(QS_SEM_TAKE, thr->m_prio)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this semaphore
            QS_2U8_PRE_(thr->m_prio, m_count);
        QS_END_PRE_()

        if (!QXK_ISR_CONTEXT_()) { // not inside ISR?
            static_cast<void>(QXK_sched_()); // schedule other threads
        }
    }
    else if (m_count < m_max_count) {
        m_count = m_count + 1U; // semaphore signaled: increment

        QS_BEGIN_PRE_(QS_SEM_SIGNAL, 0U)
            QS_TIME_PRE_();    // timestamp
            QS_OBJ_PRE_(this); // this semaphore
            QS_2U8_PRE_(0U, m_count);
        QS_END_PRE_()
    }
    else {
        signaled = false; // semaphore NOT signaled
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();

    return signaled;
}

} // namespace QP
//$enddef${QXK::QXSemaphore} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
