#pragma once

#include "_lock_types.h"

#define SPINLOCK_INIT    { .slock = 0 }

extern bool _spin_try_lock(spinlock_t *lock);
#define spin_try_lock(l)    _spin_try_lock(l)

extern void _spin_lock(spinlock_t *lock);
#define spin_lock(l)        _spin_lock(l)

extern void _spin_unlock(spinlock_t *lock);
#define spin_unlock(l)      _spin_unlock(l)

#define spinlocked(lock_ptr)                               \
	for (bool _i = (_spin_lock((lock_ptr)), true); _i; \
	     _i = (_spin_unlock((lock_ptr)), false))
