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
//! @brief platform-independent memory pool QP::QMPool interface.

#ifndef QMPOOL_HPP
#define QMPOOL_HPP

#ifndef QF_MPOOL_SIZ_SIZE
    //! macro to override the default QP::QMPoolSize size.
    //! Valid values 1U, 2U, or 4U; default 2U
    #define QF_MPOOL_SIZ_SIZE 2U
#endif

#ifndef QF_MPOOL_CTR_SIZE
    //! macro to override the default QMPoolCtr size.
    //! Valid values 1U, 2U, or 4U; default 2U
    #define QF_MPOOL_CTR_SIZE 2
#endif

namespace QP {
#if (QF_MPOOL_SIZ_SIZE == 1U)
    using QMPoolSize = std::uint8_t;
#elif (QF_MPOOL_SIZ_SIZE == 2U)
    //! The data type to store the block-size based on the macro
    //! #QF_MPOOL_SIZ_SIZE
    //!
    //! @description
    //! The dynamic range of this data type determines the maximum size
    //! of blocks that can be managed by the native QF event pool.
    using QMPoolSize = std::uint16_t;
#elif (QF_MPOOL_SIZ_SIZE == 4U)
    using QMPoolSize = std::uint32_t;
#else
    #error "QF_MPOOL_SIZ_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

#if (QF_MPOOL_CTR_SIZE == 1U)
    using QMPoolCtr = std::uint8_t;
#elif (QF_MPOOL_CTR_SIZE == 2U)
    //! The data type to store the block-counter based on the macro
    //! #QF_MPOOL_CTR_SIZE
    //!
    //! @description
    //! The dynamic range of this data type determines the maximum number
    //! of blocks that can be stored in the pool.
    using QMPoolCtr = std::uint16_t;
#elif (QF_MPOOL_CTR_SIZE == 4U)
    using QMPoolCtr = std::uint32_t;
#else
    #error "QF_MPOOL_CTR_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

//============================================================================
//! Native QF memory pool class
//!
//! @description
//! A fixed block-size memory pool is a very fast and efficient data
//! structure for dynamic allocation of fixed block-size chunks of memory.
//! A memory pool offers fast and deterministic allocation and recycling of
//! memory blocks and is not subject to fragmenation.@n
//! @n
//! The QP::QMPool class describes the native QF memory pool, which can be
//! used as the event pool for dynamic event allocation, or as a fast,
//! deterministic fixed block-size heap for any other objects in your
//! application.
//!
//! @note
//! The QP::QMPool class contains only data members for managing a memory
//! pool, but does not contain the pool storage, which must be provided
//! externally during the pool initialization.
//!
//! @note
//! The native QF event pool is configured by defining the macro
//! #QF_EPOOL_TYPE_ as QP::QMPool in the specific QF port header file.
class QMPool {
private:

    //! start of the memory managed by this memory pool
    void *m_start;

    //! end of the memory managed by this memory pool
    void *m_end;

    //! head of linked list of free blocks
    void * volatile m_free_head;

    //! maximum block size (in bytes)
    QMPoolSize m_blockSize;

    //! total number of blocks
    QMPoolCtr m_nTot;

    //! number of free blocks remaining
    QMPoolCtr volatile m_nFree;

    //! minimum number of free blocks ever present in this pool
    //!
    //! @note
    //! This attribute remembers the low watermark of the pool,
    //! which provides a valuable information for sizing event pools.
    //!
    //! @sa QP::QF::getPoolMin().
    QMPoolCtr m_nMin;

public:
    QMPool(void); //!< public default constructor

    //! Initializes the native QF event pool
    //!
    //! @description
    //! Initialize a fixed block-size memory pool by providing it with the
    //! pool memory to manage, size of this memory, and the block size.
    //!
    //! @param[in] poolSto  pointer to the memory buffer for pool storage
    //! @param[in] poolSize size of the storage buffer in bytes
    //! @param[in] blockSize fixed-size of the memory blocks in bytes
    //!
    //! @attention
    //! The caller of QP::QMPool::init() must make sure that the `poolSto`
    //! pointer is properly **aligned**. In particular, it must be possible to
    //! efficiently store a pointer at the location pointed to by `poolSto`.
    //! Internally, the QP::QMPool::init() function rounds up the block size
    //! `blockSize` so that it can fit an integer number of pointers. This
    //! is done to achieve proper alignment of the blocks within the pool.
    //!
    //! @note
    //! Due to the rounding of block size the actual capacity of the pool
    //! might be less than (@p poolSize / @p blockSize). You can check the
    //! capacity of the pool by calling the QP::QF::getPoolMin() function.
    //!
    //! @note
    //! This function is **not** protected by a critical section, because
    //! it is intended to be called only during the initialization of the
    //! system, when interrupts are not allowed yet.
    //!
    //! @note
    //! Many QF ports use memory pools to implement the event pools.
    //!
    void init(void * const poolSto, std::uint_fast32_t poolSize,
              std::uint_fast16_t blockSize) noexcept;

    //! Obtains a memory block from a memory pool
    //!
    //! @description
    //! The function allocates a memory block from the pool and returns a
    //! pointer to the block back to the caller.
    //!
    //! @param[in] margin  the minimum number of unused blocks still
    //!                    available in the pool after the allocation.
    //! @param[in] qs_id   QS-id of this state machine (for QS local filter)
    //!
    //! @returns
    //! A pointer to a memory block or NULL if no more blocks are available
    //! in  the memory pool.
    //!
    //! @note
    //! This function can be called from any task level or ISR level.
    //!
    //! @note
    //! The memory pool must be initialized before any events can
    //! be requested from it. Also, the QP::QMPool::get() function uses
    //! internally a QF critical section, so you should be careful not to
    //! call it from within a critical section when nesting of critical
    //! section is not supported.
    //!
    //! @attention
    //! An allocated block must be later returned back to the **same** pool
    //! from which it has been allocated.
    //!
    //! @sa
    //! QP::QMPool::put()
    //!
    void *get(std::uint_fast16_t const margin,
              std::uint_fast8_t const qs_id) noexcept;

    //! Returns a memory block back to a memory pool
    //!
    //! @description
    //! Recycle a memory block to the fixed block-size memory pool.
    //!
    //! @param[in] b     pointer to the memory block that is being recycled
    //! @param[in] qs_id QS-id of this state machine (for QS local filter)
    //!
    //! @attention
    //! The recycled block must be allocated from the **same** memory pool
    //! to which it is returned.
    //!
    //! @note
    //! This function can be called from any task level or ISR level.
    //!
    //! @sa
    //! QP::QMPool::get()
    //!
    void put(void * const b, std::uint_fast8_t const qs_id) noexcept;

    //! return the fixed block-size of the blocks managed by this pool
    QMPoolSize getBlockSize(void) const noexcept {
        return m_blockSize;
    }

// duplicated API to be used exclusively inside ISRs (useful in some QP ports)
#ifdef QF_ISR_API
    void *getFromISR(std::uint_fast16_t const margin,
                     std::uint_fast8_t const qs_id) noexcept;
    void putFromISR(void * const b,
                    std::uint_fast8_t const qs_id) noexcept;
#endif // QF_ISR_API

private:
    //! disallow copying of QMPools
    QMPool(QMPool const &) = delete;

    //!< disallow assigning of QMPools
    QMPool &operator=(QMPool const &) = delete;

    friend class QF;
    friend class QS;
};

} // namespace QP

//! Memory pool element to allocate correctly aligned storage for QP::QMPool
#define QF_MPOOL_EL(type_) \
    struct { void *sto_[((sizeof(type_) - 1U)/sizeof(void*)) + 1U]; }

#endif  // QMPOOL_HPP
