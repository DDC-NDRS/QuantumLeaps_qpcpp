//$file${src::blinky.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: blinky.qm
// File:  ${src::blinky.cpp}
//
// This code has been generated by QM 5.2.0 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This generated code is open source software: you can redistribute it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation.
//
// This code is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// NOTE:
// Alternatively, this generated code may be distributed under the terms
// of Quantum Leaps commercial licenses, which expressly supersede the GNU
// General Public License and are specifically designed for licensees
// interested in retaining the proprietary status of their code.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//
//$endhead${src::blinky.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#include "qpcpp.hpp"  // QP/C++ framework API
#include "blinky.hpp" // Blinky application
#include "bsp.hpp"    // Board Support Package interface

//using namespace QP;

// ask QM to declare the Blinky class ----------------------------------------
//$declare${AOs::Blinky} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${AOs::Blinky} .............................................................
class Blinky : public QP::QActive {
private:
    QP::QTimeEvt m_timeEvt;

public:
    static Blinky inst;

public:
    Blinky();

protected:
    Q_STATE_DECL(initial);
    Q_STATE_DECL(off);
    Q_STATE_DECL(on);
}; // class Blinky
//$enddecl${AOs::Blinky} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// define the global opaque pointer to Blinky AO -----------------------------
//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 690U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 6.9.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${AOs::AO_Blinky} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${AOs::AO_Blinky} ..........................................................
QP::QActive * const AO_Blinky = &Blinky::inst;
//$enddef${AOs::AO_Blinky} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ask QM to define the Blinky class (including the state machine) -----------
//$define${AOs::Blinky} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${AOs::Blinky} .............................................................
Blinky Blinky::inst;

//${AOs::Blinky::Blinky} .....................................................
Blinky::Blinky()
  : QActive(&initial),
    m_timeEvt(this, TIMEOUT_SIG, 0U)
{}

//${AOs::Blinky::SM} .........................................................
Q_STATE_DEF(Blinky, initial) {
    //${AOs::Blinky::SM::initial}
    QS_OBJ_DICTIONARY(&Blinky::inst);
    QS_OBJ_DICTIONARY(&Blinky::inst.m_timeEvt);

    // arm the private time event to expire in 1/2s
    // and periodically every 1/2 second
    m_timeEvt.armX(BSP::TICKS_PER_SEC/2,
                   BSP::TICKS_PER_SEC/2);
    (void)e; // unused parameter

    QS_FUN_DICTIONARY(&Blinky::off);
    QS_FUN_DICTIONARY(&Blinky::on);

    return tran(&off);
}

//${AOs::Blinky::SM::off} ....................................................
Q_STATE_DEF(Blinky, off) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Blinky::SM::off}
        case Q_ENTRY_SIG: {
            BSP::ledOff();
            status_ = Q_RET_HANDLED;
            break;
        }
        //${AOs::Blinky::SM::off::TIMEOUT}
        case TIMEOUT_SIG: {
            status_ = tran(&on);
            break;
        }
        default: {
            status_ = super(&top);
            break;
        }
    }
    return status_;
}

//${AOs::Blinky::SM::on} .....................................................
Q_STATE_DEF(Blinky, on) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Blinky::SM::on}
        case Q_ENTRY_SIG: {
            BSP::ledOn();
            status_ = Q_RET_HANDLED;
            break;
        }
        //${AOs::Blinky::SM::on::TIMEOUT}
        case TIMEOUT_SIG: {
            status_ = tran(&off);
            break;
        }
        default: {
            status_ = super(&top);
            break;
        }
    }
    return status_;
}
//$enddef${AOs::Blinky} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
