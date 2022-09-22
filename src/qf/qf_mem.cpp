//$file${src::qf::qf_mem.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qf::qf_mem.cpp}
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
//$endhead${src::qf::qf_mem.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//! @file
//! @brief QF/C++ memory management services

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
Q_DEFINE_THIS_MODULE("qf_mem")
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 690U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 6.9.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QF::QMPool} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QF::QMPool} ..............................................................

//${QF::QMPool::QMPool} ......................................................
QMPool::QMPool()
  : m_start(nullptr),
    m_end(nullptr),
    m_free_head(nullptr),
    m_blockSize(0U),
    m_nTot(0U),
    m_nFree(0U),
    m_nMin(0U)
{}

//${QF::QMPool::init} ........................................................
void QMPool::init(
    void * const poolSto,
    std::uint_fast32_t poolSize,
    std::uint_fast16_t blockSize) noexcept
{
    //! @pre The memory block must be valid and
    //! the poolSize must fit at least one free block and
    //! the blockSize must not be too close to the top of the dynamic range
    Q_REQUIRE_ID(100, (poolSto != nullptr)
        && (poolSize >= static_cast<std::uint_fast32_t>(sizeof(QFreeBlock)))
        && (static_cast<std::uint_fast16_t>(blockSize + sizeof(QFreeBlock))
            > blockSize));

    m_free_head = poolSto;

    // round up the blockSize to fit an integer number of pointers...
    //start with one
    m_blockSize = static_cast<QMPoolSize>(sizeof(QFreeBlock));

    //# free blocks in a memory block
    std::uint_fast16_t nblocks = 1U;
    while (m_blockSize < static_cast<QMPoolSize>(blockSize)) {
        m_blockSize += static_cast<QMPoolSize>(sizeof(QFreeBlock));
        ++nblocks;
    }
    // use rounded-up value
    blockSize = static_cast<std::uint_fast16_t>(m_blockSize);

    // the whole pool buffer must fit at least one rounded-up block
    Q_ASSERT_ID(110, poolSize >= blockSize);

    // chain all blocks together in a free-list...

    // don't count the last block
    poolSize -= static_cast<std::uint_fast32_t>(blockSize);
    m_nTot = 1U; // one (the last) block in the pool

    // start at the head of the free list
    QFreeBlock *fb = static_cast<QFreeBlock *>(m_free_head);

    // chain all blocks together in a free-list...
    while (poolSize >= blockSize) {
        fb->m_next = &fb[nblocks]; // setup the next link
        fb = fb->m_next;  // advance to next block
        // reduce the available pool size
        poolSize -= static_cast<std::uint_fast32_t>(blockSize);
        ++m_nTot;         // increment the number of blocks so far
    }

    fb->m_next = nullptr; // the last link points to NULL
    m_nFree    = m_nTot;  // all blocks are free
    m_nMin     = m_nTot;  // the minimum number of free blocks
    m_start    = poolSto; // the original start this pool buffer
    m_end      = fb;      // the last block in this pool
}

//${QF::QMPool::get} .........................................................
void * QMPool::get(
    std::uint_fast16_t const margin,
    std::uint_fast8_t const qs_id) noexcept
{
    Q_UNUSED_PAR(qs_id); // when Q_SPY not defined

    QF_CRIT_STAT_
    QF_CRIT_E_();

    // have the than margin?
    QFreeBlock *fb;
    if (m_nFree > static_cast<QMPoolCtr>(margin)) {
        fb = static_cast<QFreeBlock *>(m_free_head);  // get a free block

        // the pool has some free blocks, so a free block must be available
        Q_ASSERT_CRIT_(310, fb != nullptr);

        // put volatile to a temporary to avoid UB
        void * const fb_next = fb->m_next;

        // is the pool becoming empty?
        m_nFree = (m_nFree - 1U); // one free block less
        if (m_nFree == 0U) {
            // pool is becoming empty, so the next free block must be NULL
            Q_ASSERT_CRIT_(320, fb_next == nullptr);

            m_nMin = 0U;// remember that pool got empty
        }
        else {
            // pool is not empty, so the next free block must be in range
            //
            // NOTE: the next free block pointer can fall out of range
            // when the client code writes past the memory block, thus
            // corrupting the next block.
            Q_ASSERT_CRIT_(330, QF_PTR_RANGE_(fb_next, m_start, m_end));

            // is the number of free blocks the new minimum so far?
            if (m_nMin > m_nFree) {
                m_nMin = m_nFree; // remember the minimum so far
            }
        }

        m_free_head = fb_next; // set the head to the next free block

        QS_BEGIN_NOCRIT_PRE_(QS_QF_MPOOL_GET, qs_id)
            QS_TIME_PRE_();        // timestamp
            QS_OBJ_PRE_(this);     // this memory pool
            QS_MPC_PRE_(m_nFree);  // # of free blocks in the pool
            QS_MPC_PRE_(m_nMin);   // min # free blocks ever in the pool
        QS_END_NOCRIT_PRE_()
    }
    // don't have enough free blocks at this point
    else {
        fb = nullptr;

        QS_BEGIN_NOCRIT_PRE_(QS_QF_MPOOL_GET_ATTEMPT, qs_id)
            QS_TIME_PRE_();        // timestamp
            QS_OBJ_PRE_(m_start);  // the memory managed by this pool
            QS_MPC_PRE_(m_nFree);  // the # free blocks in the pool
            QS_MPC_PRE_(margin);   // the requested margin
        QS_END_NOCRIT_PRE_()
    }
    QF_CRIT_X_();

    return fb; // return the block or NULL pointer to the caller

}

//${QF::QMPool::put} .........................................................
void QMPool::put(
    void * const b,
    std::uint_fast8_t const qs_id) noexcept
{
    Q_UNUSED_PAR(qs_id); // when Q_SPY not defined

    //! @pre # free blocks cannot exceed the total # blocks and
    //! the block pointer must be in range to come from this pool.
    //!
    Q_REQUIRE_ID(200, (m_nFree < m_nTot)
                      && QF_PTR_RANGE_(b, m_start, m_end));
    QF_CRIT_STAT_
    QF_CRIT_E_();
    static_cast<QFreeBlock*>(b)->m_next =
        static_cast<QFreeBlock *>(m_free_head); // link into the free list
    m_free_head = b; // set as new head of the free list
    m_nFree = (m_nFree + 1U); // one more free block in this pool

    QS_BEGIN_NOCRIT_PRE_(QS_QF_MPOOL_PUT, qs_id)
        QS_TIME_PRE_();       // timestamp
        QS_OBJ_PRE_(this);    // this memory pool
        QS_MPC_PRE_(m_nFree); // the number of free blocks in the pool
    QS_END_NOCRIT_PRE_()

    QF_CRIT_X_();
}

//${QF::QMPool::getBlockSize} ................................................
QMPoolSize QMPool::getBlockSize() const noexcept {
    return m_blockSize;
}

} // namespace QP
//$enddef${QF::QMPool} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
