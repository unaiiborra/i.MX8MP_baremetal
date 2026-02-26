#pragma once

#include "_lock_types.h"

/// It is not an spinlock, only disables irqs and returns the daif state
extern irqlock_t _irq_lock(void);
#define irq_lock() _irq_lock()
/// It is not an spinlock, only restores the previous state of daif
extern void _irq_unlock(irqlock_t l);
#define irq_unlock(l) _irq_unlock(l)

// locks the inside scope. Returning or breaking from this scope will not unlock
// the lock.
#define irqlocked()                                         \
    for (irqlock_t _i = {true}, _f = _irq_lock(); _i.flags; \
         _i = (_irq_unlock(_f), (irqlock_t) {false}))
