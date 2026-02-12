#pragma once

#include "_lock_types.h"

extern irqlock_t _spin_lock_irqsave(spinlock_t* lock);

#define spin_lock_irqsave(lock) _spin_lock_irqsave(lock)

extern void _spin_unlock_irqrestore(spinlock_t* lock, irqlock_t flags);
#define spin_unlock_irqrestore(lock, flags) _spin_unlock_irqrestore(lock, flags)

#define irq_spinlocked(lock_ptr)                                                   \
    for (irqlock_t _i = {true}, _flags = _spin_lock_irqsave((lock_ptr)); _i.flags; \
         _i = (_spin_unlock_irqrestore((lock_ptr), _flags), (irqlock_t) {false}))
