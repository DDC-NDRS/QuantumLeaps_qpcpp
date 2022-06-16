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
//! @brief QF/C++ time events and time management services

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

Q_DEFINE_THIS_MODULE("qf_time")

} // unnamed namespace

//============================================================================
namespace QP {

// Package-scope objects .....................................................
QTimeEvt QF::timeEvtHead_[QF_MAX_TICK_RATE]; // heads of time event lists

#ifdef Q_SPY

//............................................................................
void QF::tickX_(std::uint_fast8_t const tickRate,
                void const * const sender) noexcept
#else
void QF::tickX_(std::uint_fast8_t const tickRate) noexcept
#endif
{
    QTimeEvt *prev = &timeEvtHead_[tickRate];

    QF_CRIT_STAT_
    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_TICK, 0U)
        prev->m_ctr = (prev->m_ctr + 1U);
        QS_TEC_PRE_(prev->m_ctr); // tick ctr
        QS_U8_PRE_(tickRate);     // tick rate
    QS_END_NOCRIT_PRE_()

    // scan the linked-list of time events at this rate...
    for (;;) {
        QTimeEvt *t = prev->m_next; // advance down the time evt. list

        // end of the list?
        if (t == nullptr) {

            // any new time events armed since the last run of QF::tickX_()?
            if (timeEvtHead_[tickRate].m_act != nullptr) {

                // sanity check
                Q_ASSERT_CRIT_(110, prev != nullptr);
                prev->m_next = QF::timeEvtHead_[tickRate].toTimeEvt();
                timeEvtHead_[tickRate].m_act = nullptr;
                t = prev->m_next; // switch to the new list
            }
            else {
                break; // all currently armed time evts. processed
            }
        }

        // time event scheduled for removal?
        if (t->m_ctr == 0U) {
            prev->m_next = t->m_next;
            // mark time event 't' as NOT linked
            t->refCtr_ = static_cast<std::uint8_t>(t->refCtr_
                & static_cast<std::uint8_t>(~TE_IS_LINKED));
            // do NOT advance the prev pointer
            QF_CRIT_X_(); // exit crit. section to reduce latency

            // prevent merging critical sections, see NOTE1 below
            QF_CRIT_EXIT_NOP();
        }
        else {
            t->m_ctr = (t->m_ctr - 1U);

            // is time evt about to expire?
            if (t->m_ctr == 0U) {
                QActive * const act = t->toActive(); // temp for volatile

                // periodic time evt?
                if (t->m_interval != 0U) {
                    t->m_ctr = t->m_interval; // rearm the time event
                    prev = t; // advance to this time event
                }
                // one-shot time event: automatically disarm
                else {
                    prev->m_next = t->m_next;

                    // mark time event 't' as NOT linked
                    t->refCtr_ = static_cast<std::uint8_t>(t->refCtr_
                        & static_cast<std::uint8_t>(~TE_IS_LINKED));
                    // do NOT advance the prev pointer

                    QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_AUTO_DISARM,
                                         act->m_prio)
                        QS_OBJ_PRE_(t);       // this time event object
                        QS_OBJ_PRE_(act);     // the target AO
                        QS_U8_PRE_(tickRate); // tick rate
                    QS_END_NOCRIT_PRE_()
                }

                QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_POST, act->m_prio)
                    QS_TIME_PRE_();       // timestamp
                    QS_OBJ_PRE_(t);       // the time event object
                    QS_SIG_PRE_(t->sig);  // signal of this time event
                    QS_OBJ_PRE_(act);     // the target AO
                    QS_U8_PRE_(tickRate); // tick rate
                QS_END_NOCRIT_PRE_()

                QF_CRIT_X_(); // exit crit. section before posting

                // asserts if queue overflows
                static_cast<void>(act->POST(t, sender));
            }
            else {
                prev = t; // advance to this time event
                QF_CRIT_X_(); // exit crit. section to reduce latency

                // prevent merging critical sections, see NOTE1 below
                QF_CRIT_EXIT_NOP();
            }
        }
        QF_CRIT_E_(); // re-enter crit. section to continue
    }
    QF_CRIT_X_();
}

//............................................................................
bool QF::noTimeEvtsActiveX(std::uint_fast8_t const tickRate) noexcept {

    //! @pre the tick rate must be in range
    Q_REQUIRE_ID(200, tickRate < QF_MAX_TICK_RATE);

    bool inactive;
    if (timeEvtHead_[tickRate].m_next != nullptr) {
        inactive = false;
    }
    else if (timeEvtHead_[tickRate].m_act != nullptr) {
        inactive = false;
    }
    else {
        inactive = true;
    }
    return inactive;
}

//............................................................................
//! @description
//! When creating a time event, you must commit it to a specific active object
//! @p act, tick rate @p tickRate and event signal @p sgnl. You cannot change
//! these attributes later.
//!
//! @param[in] act    pointer to the active object associated with this
//!                   time event. The time event will post itself to this AO.
//! @param[in] sgnl   signal to associate with this time event.
//! @param[in] tickRate system tick rate to associate with this time event.
//!
QTimeEvt::QTimeEvt(QActive * const act,
    enum_t const sgnl, std::uint_fast8_t const tickRate) noexcept
    :
#ifdef Q_EVT_CTOR
    QEvt(static_cast<QSignal>(sgnl), QEvt::STATIC_EVT),
#else
    QEvt(),
#endif
    m_next(nullptr),
    m_act(act),
    m_ctr(0U),
    m_interval(0U)
{
    //! @pre The signal must be valid and the tick rate in range
    Q_REQUIRE_ID(300, (sgnl >= Q_USER_SIG)
                      && (tickRate < QF_MAX_TICK_RATE));

#ifndef Q_EVT_CTOR
    sig = static_cast<QSignal>(sgnl); // set QEvt::sig of this time event
#endif

    // Setting the POOL_ID event attribute to zero is correct only for
    // events not allocated from event pools, which must be the case
    // for Time Events.
    //
    poolId_ = 0U;

    // The refCtr_ attribute is not used in time events, so it is
    // reused to hold the tickRate as well as other information
    //
    refCtr_ = static_cast<std::uint8_t>(tickRate);
}

//............................................................................
QTimeEvt::QTimeEvt() noexcept
    :
#ifdef Q_EVT_CTOR
    QEvt(0U, QEvt::STATIC_EVT),
#else
    QEvt(),
#endif // Q_EVT_CTOR
    m_next(nullptr),
    m_act(nullptr),
    m_ctr(0U),
    m_interval(0U)
{
#ifndef Q_EVT_CTOR
    sig = 0U;

    // Setting the POOL_ID event attribute to zero is correct only for
    // events not allocated from event pools, which must be the case
    // for Time Events.
    //
    poolId_ = 0U; // not from any event pool

    // The refCtr_ attribute is not used in time events, so it is
    // reused to hold the tickRate as well as other information
    //
    refCtr_ = 0U; // default rate 0

#endif  // Q_EVT_CTOR

}

//............................................................................
void QTimeEvt::armX(QTimeEvtCtr const nTicks,
                    QTimeEvtCtr const interval) noexcept
{
    std::uint8_t const tickRate = refCtr_ & TE_TICK_RATE;
    QTimeEvtCtr const ctr = m_ctr;  // temporary to hold volatile

    //! @pre the host AO must be valid, time evnet must be disarmed,
    //! number of clock ticks cannot be zero, and the signal must be valid.
    //!
    Q_REQUIRE_ID(400, (m_act != nullptr)
       && (ctr == 0U)
       && (nTicks != 0U)
       && (tickRate < static_cast<std::uint8_t>(QF_MAX_TICK_RATE))
       && (static_cast<enum_t>(sig) >= Q_USER_SIG));
#ifdef Q_NASSERT
    (void)ctr; // avoid compiler warning about unused variable
#endif

    QF_CRIT_STAT_
    QF_CRIT_E_();
    m_ctr = nTicks;
    m_interval = interval;

    // is the time event unlinked?
    // NOTE: For the duration of a single clock tick of the specified tick
    // rate a time event can be disarmed and yet still linked into the list,
    // because un-linking is performed exclusively in the QF_tickX() function.
    //
    if (static_cast<std::uint_fast8_t>(
           static_cast<std::uint_fast8_t>(refCtr_) & TE_IS_LINKED) == 0U)
    {
        // mark as linked
        refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_IS_LINKED);

        // The time event is initially inserted into the separate
        // "freshly armed" link list based on QF::timeEvtHead_[tickRate].act.
        // Only later, inside the QF::tickX() function, the "freshly armed"
        // list is appended to the main list of armed time events based on
        // QF::timeEvtHead_[tickRate].next. Again, this is to keep any
        // changes to the main list exclusively inside the QF::tickX()
        // function.
        //
        m_next = QF::timeEvtHead_[tickRate].toTimeEvt();
        QF::timeEvtHead_[tickRate].m_act = this;
    }

#ifdef Q_SPY
    std::uint_fast8_t const qs_id = static_cast<QActive *>(m_act)->m_prio;
#endif
    QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_ARM, qs_id)
        QS_TIME_PRE_();        // timestamp
        QS_OBJ_PRE_(this);     // this time event object
        QS_OBJ_PRE_(m_act);    // the active object
        QS_TEC_PRE_(nTicks);   // the number of ticks
        QS_TEC_PRE_(interval); // the interval
        QS_U8_PRE_(tickRate);  // tick rate
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();
}

//............................................................................
bool QTimeEvt::disarm(void) noexcept {
    QF_CRIT_STAT_
    QF_CRIT_E_();
#ifdef Q_SPY
    std::uint_fast8_t const qs_id = static_cast<QActive *>(m_act)->m_prio;
#endif

    // is the time event actually armed?
    bool wasArmed;
    if (m_ctr != 0U) {
        wasArmed = true;
        refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_WAS_DISARMED);

        QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_DISARM, qs_id)
            QS_TIME_PRE_();            // timestamp
            QS_OBJ_PRE_(this);         // this time event object
            QS_OBJ_PRE_(m_act);        // the target AO
            QS_TEC_PRE_(m_ctr);        // the number of ticks
            QS_TEC_PRE_(m_interval);   // the interval
            QS_U8_PRE_(refCtr_& TE_TICK_RATE);
        QS_END_NOCRIT_PRE_()

        m_ctr = 0U; // schedule removal from the list
    }
    else { // the time event was already disarmed automatically
        wasArmed = false;
        refCtr_ = static_cast<std::uint8_t>(refCtr_
            & static_cast<std::uint8_t>(~TE_WAS_DISARMED));

        QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_DISARM_ATTEMPT, qs_id)
            QS_TIME_PRE_();            // timestamp
            QS_OBJ_PRE_(this);         // this time event object
            QS_OBJ_PRE_(m_act);        // the target AO
            QS_U8_PRE_(refCtr_& TE_TICK_RATE); // tick rate
        QS_END_NOCRIT_PRE_()

    }
    QF_CRIT_X_();

    return wasArmed;
}

//............................................................................
bool QTimeEvt::rearm(QTimeEvtCtr const nTicks) noexcept {
    std::uint8_t const tickRate = refCtr_ & TE_TICK_RATE;

    //! @pre AO must be valid, tick rate must be in range, nTicks must not
    //! be zero, and the signal of this time event must be valid
    //!
    Q_REQUIRE_ID(600, (m_act != nullptr)
        && (tickRate < static_cast<std::uint8_t>(QF_MAX_TICK_RATE))
        && (nTicks != 0U)
        && (static_cast<enum_t>(sig) >= Q_USER_SIG));

    QF_CRIT_STAT_
    QF_CRIT_E_();

    // is the time evt not running?
    bool wasArmed;
    if (m_ctr == 0U) {
        wasArmed = false;

        // is the time event unlinked?
        // NOTE: For a duration of a single clock tick of the specified
        // tick rate a time event can be disarmed and yet still linked into
        // the list, because unlinking is performed exclusively in the
        // QF::tickX() function.
        //
        if (static_cast<std::uint8_t>(refCtr_ & TE_IS_LINKED) == 0U) {
            // mark as linked
            refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_IS_LINKED);

            // The time event is initially inserted into the separate
            // "freshly armed" list based on QF_timeEvtHead_[tickRate].act.
            // Only later, inside the QF_tickX() function, the "freshly armed"
            // list is appended to the main list of armed time events based on
            // QF_timeEvtHead_[tickRate].next. Again, this is to keep any
            // changes to the main list exclusively inside the QF::tickX()
            // function.
            //
            m_next = QF::timeEvtHead_[tickRate].toTimeEvt();
            QF::timeEvtHead_[tickRate].m_act = this;
        }
    }
    else { // the time event is being disarmed
        wasArmed = true;
    }
    m_ctr = nTicks; // re-load the tick counter (shift the phasing)

#ifdef Q_SPY
    std::uint_fast8_t const qs_id = static_cast<QActive *>(m_act)->m_prio;
#endif
    QS_BEGIN_NOCRIT_PRE_(QS_QF_TIMEEVT_REARM, qs_id)
        QS_TIME_PRE_();          // timestamp
        QS_OBJ_PRE_(this);       // this time event object
        QS_OBJ_PRE_(m_act);      // the target AO
        QS_TEC_PRE_(m_ctr);      // the number of ticks
        QS_TEC_PRE_(m_interval); // the interval
        QS_2U8_PRE_(tickRate, (wasArmed ? 1U : 0U));
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();

    return wasArmed;
}

//............................................................................
bool QTimeEvt::wasDisarmed(void) noexcept {
    std::uint8_t const isDisarmed = refCtr_ & TE_WAS_DISARMED;
    // mark as disarmed
    refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_WAS_DISARMED);
    return isDisarmed != 0U;
}

//............................................................................
//! @description
//! Useful for checking how many clock ticks (at the tick rate associated
//! with the time event) remain until the time event expires.
//!
//! @returns
//! For an armed time event, the function returns the current value of the
//! down-counter of the given time event. If the time event is not armed,
//! the function returns 0.
//!
//! @note
//! The function is thread-safe.
//!
QTimeEvtCtr QTimeEvt::currCtr(void) const noexcept {
    QF_CRIT_STAT_
    QF_CRIT_E_();
    QTimeEvtCtr const ret = m_ctr;
    QF_CRIT_X_();

    return ret;
}

} // namespace QP

//============================================================================
// NOTE1:
// In some QF ports the critical section exit takes effect only on the next
// machine instruction. If this case, the next instruction is another entry
// to a critical section, the critical section won't be really exited, but
// rather the two adjacent critical sections would be merged.
//
// The QF_CRIT_EXIT_NOP() macro contains minimal code required
// to prevent such merging of critical sections in QF ports,
// in which it can occur.
