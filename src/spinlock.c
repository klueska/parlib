/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 *
 * This file is part of Parlib.
 * 
 * Parlib is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Parlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * See COPYING.LESSER for details on the GNU Lesser General Public License.
 * See COPYING for details on the GNU General Public License.
 */

#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "spinlock.h"
#include "atomic.h"
#include "arch.h"

void spinlock_init(spinlock_t *lock)
{
  assert(lock);
  *lock = SPINLOCK_UNLOCKED;
}


int spinlock_trylock(spinlock_t *lock) 
{
  assert(lock);
  if (*lock == SPINLOCK_LOCKED)
    return SPINLOCK_LOCKED;

  return cmpxchg(lock, SPINLOCK_LOCKED, SPINLOCK_UNLOCKED);
}


void spinlock_lock(spinlock_t *lock) 
{
  assert(lock);
  while (spinlock_trylock(lock) != SPINLOCK_UNLOCKED)
    cpu_relax();
}


void spinlock_unlock(spinlock_t *lock) 
{
  assert(lock);
  *lock = SPINLOCK_UNLOCKED;
}
