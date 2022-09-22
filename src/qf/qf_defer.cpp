//$file${src::qf::qf_defer.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qf::qf_defer.cpp}
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
//$endhead${src::qf::qf_defer.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//! @file
//! @brief QActive::defer(), QActive::recall(), and
//! QActive::flushDeferred() definitions.

#define QP_IMPL             // this is QP implementation
#include "qf_port.hpp"      // QF port
#include "qf_pkg.hpp"       // QF package-scope interface
#include "qassert.h"        // QP embedded systems-friendly assertions
#ifdef Q_SPY                // QS software tracing enabled?
    #include "qs_port.hpp"  // QS port
    #include "qs_pkg.hpp"   // QS facilities for pre-defined trace records
#else
    #include "qs_dummy.hpp" // disable the QS software tracing
#endif // Q_SPY

// unnamed namespace for local definitions with internal linkage
namespace {
Q_DEFINE_THIS_MODULE("qf_defer")
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 690U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 6.9.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QF::QActive::defer} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QF::QActive::defer} ......................................................
bool QActive::defer(
    QEQueue * const eq,
    QEvt const * const e) const noexcept
{
    bool const status = eq->post(e, 0U, m_prio);
    QS_CRIT_STAT_

    QS_BEGIN_PRE_(QS_QF_ACTIVE_DEFER, m_prio)
        QS_TIME_PRE_();      // time stamp
        QS_OBJ_PRE_(this);   // this active object
        QS_OBJ_PRE_(eq);     // the deferred queue
        QS_SIG_PRE_(e->sig); // the signal of the event
        QS_2U8_PRE_(e->poolId_, e->refCtr_); // pool Id & ref Count
    QS_END_PRE_()

    return status;
}

} // namespace QP
//$enddef${QF::QActive::defer} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$define${QF::QActive::recall} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QF::QActive::recall} .....................................................
bool QActive::recall(QEQueue * const eq) noexcept {
    QEvt const * const e = eq->get(m_prio); // get evt from deferred queue
    bool recalled;

    // event available?
    if (e != nullptr) {
        QActive::postLIFO(e); // post it to the _front_ of the AO's queue

        QF_CRIT_STAT_
        QF_CRIT_E_();

        // is it a dynamic event?
        if (e->poolId_ != 0U) {

            // after posting to the AO's queue the event must be referenced
            // at least twice: once in the deferred event queue (eq->get()
            // did NOT decrement the reference counter) and once in the
            // AO's event queue.
            Q_ASSERT_CRIT_(210, e->refCtr_ >= 2U);

            // we need to decrement the reference counter once, to account
            // for removing the event from the deferred event queue.
            QF_EVT_REF_CTR_DEC_(e); // decrement the reference counter
        }

        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_RECALL, m_prio)
            QS_TIME_PRE_();      // time stamp
            QS_OBJ_PRE_(this);   // this active object
            QS_OBJ_PRE_(eq);     // the deferred queue
            QS_SIG_PRE_(e->sig); // the signal of the event
            QS_2U8_PRE_(e->poolId_, e->refCtr_); // pool Id & ref Count
        QS_END_NOCRIT_PRE_()

        QF_CRIT_X_();
        recalled = true;
    }
    else {
        QS_CRIT_STAT_

        QS_BEGIN_PRE_(QS_QF_ACTIVE_RECALL_ATTEMPT, m_prio)
            QS_TIME_PRE_();      // time stamp
            QS_OBJ_PRE_(this);   // this active object
            QS_OBJ_PRE_(eq);     // the deferred queue
        QS_END_PRE_()

        recalled = false;
    }
    return recalled;

}

} // namespace QP
//$enddef${QF::QActive::recall} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$define${QF::QActive::flushDeferred} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QF::QActive::flushDeferred} ..............................................
std::uint_fast16_t QActive::flushDeferred(QEQueue * const eq) const noexcept {
    std::uint_fast16_t n = 0U;
    for (QEvt const *e = eq->get(m_prio);
         e != nullptr;
         e = eq->get(m_prio))
    {
        ++n; // count the flushed event
    #if (QF_MAX_EPOOL > 0U)
        QF::gc(e); // garbage collect
    #endif
    }
    return n;
}

} // namespace QP
//$enddef${QF::QActive::flushDeferred} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
