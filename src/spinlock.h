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

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <errno.h>
#include <string.h>
#include "uthread.h"
#include "atomic.h"
#include "arch.h"

#define SPINLOCK_INITIALIZER {0}
#define SPINPDR_INITIALIZER {0}

typedef struct spinlock {
  int lock;
} spinlock_t;

typedef struct spin_pdr_lock {
  int lock;
} spin_pdr_lock_t;

typedef struct {
  size_t num_vcores;
  volatile size_t count;
  volatile char CACHE_LINE_ALIGNED sense;
  char CACHE_LINE_ALIGNED local_sense[ARCH_CL_SIZE * MAX_VCORES];
} spin_barrier_t;

static inline void spinlock_init(spinlock_t *lock)
{
  lock->lock = 0;
}

static inline int spinlock_trylock(spinlock_t *lock)
{
  return __sync_lock_test_and_set(&lock->lock, EBUSY);
}

static inline void spinlock_lock(spinlock_t *lock)
{
  while (spinlock_trylock(lock))
    cpu_relax();
}

static inline void spinlock_unlock(spinlock_t *lock)
{
  __sync_lock_release(&lock->lock, 0);
}

static void spin_pdr_init(struct spin_pdr_lock *pdr_lock)
{
  pdr_lock->lock = 0;
}

static void spin_pdr_lock(struct spin_pdr_lock *pdr_lock)
{
  if (!in_vcore_context() && current_uthread)
    uth_disable_notifs();
  spinlock_lock((spinlock_t*)pdr_lock);
}

static void spin_pdr_unlock(struct spin_pdr_lock *pdr_lock)
{
  spinlock_unlock((spinlock_t*)pdr_lock);
  if (!in_vcore_context() && current_uthread)
    uth_enable_notifs();
}

static void spin_barrier_init(spin_barrier_t *b, size_t num_vcores)
{
  b->num_vcores = num_vcores;
  b->count = num_vcores;
  b->sense = 0;
  memset(b->local_sense, 0, ARCH_CL_SIZE * MAX_VCORES);
}

static void spin_barrier_wait(spin_barrier_t *b)
{
  char *local_sense = b->local_sense + ARCH_CL_SIZE * vcore_id();
  char current_local_sense = !*local_sense;
  *local_sense = current_local_sense;

  if (__sync_fetch_and_add(&b->count, -1) == 1) {
    b->count = b->num_vcores;
    b->sense = current_local_sense;
  } else {
    while (b->sense != current_local_sense)
      cpu_relax();
  }
}

#endif // SPINLOCK_H
