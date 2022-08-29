//$file${include::qv.hpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${include::qv.hpp}
//
// This code has been generated by QM 5.2.1 <www.state-machine.com/qm>.
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
//$endhead${include::qv.hpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//! @file
//! @brief QV/C++ platform-independent public interface.

#ifndef QV_HPP
#define QV_HPP

//============================================================================
// QF customization for QV -- data members of the QActive class...

// QV event-queue used for AOs
#define QF_EQUEUE_TYPE  QEQueue

//============================================================================
#include "qequeue.hpp" // QV kernel uses the native QF event queue
#include "qmpool.hpp"  // QV kernel uses the native QF memory pool
#include "qf.hpp"      // QF framework integrates directly with QV

//============================================================================
//$declare${QV::QV-base} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {
namespace QV {

//${QV::QV-base::onIdle} .....................................................
//! QV idle callback (customized in BSPs for QV)
//!
//! @attention
//! QV::onIdle() must be called with interrupts DISABLED because the
//! determination of the idle condition (no events in the queues) can
//! change at any time by an interrupt posting events to a queue.
//! QV::onIdle() MUST enable interrupts internally, ideally **atomically**
//! with putting the CPU into a power-saving mode.
void onIdle() ;

} // namespace QV
} // namespace QP
//$enddecl${QV::QV-base} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//============================================================================
// interface used only inside QF, but not in applications
#ifdef QP_IMPL
// QV-specific scheduler locking and event queue...
//$declare${QV-impl} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QV-impl::QF_SCHED_STAT_} .................................................
//! QV scheduler lock status (not needed in QV)
#define QF_SCHED_STAT_

//${QV-impl::QF_SCHED_LOCK_} .................................................
//! QV scheduler locking (not needed in QV)
#define QF_SCHED_LOCK_(dummy) (static_cast<void>(0))

//${QV-impl::QF_SCHED_UNLOCK_} ...............................................
//! QV scheduler unlocking (not needed in QV)
#define QF_SCHED_UNLOCK_() (static_cast<void>(0))

//${QV-impl::QACTIVE_EQUEUE_WAIT_} ...........................................
//! QV native event queue waiting
#define QACTIVE_EQUEUE_WAIT_(me_) \
    Q_ASSERT_ID(110, (me_)->m_eQueue.m_frontEvt != nullptr)

//${QV-impl::QACTIVE_EQUEUE_SIGNAL_} .........................................
//! QV native event queue signaling
#define QACTIVE_EQUEUE_SIGNAL_(me_) \
    (QF::readySet_.insert(static_cast<std::uint_fast8_t>((me_)->m_prio)))
//$enddecl${QV-impl} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
#endif // QV_HPP
