/*
 * Copyright (c) 2011 The Regents of the University of California
 * Andrew Waterman <waterman@cs.berkeley.edu>
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

#ifndef _MCS_H
#define _MCS_H

#include "vcore.h"
#include "arch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCS_LOCK_INIT {0}
#define MCS_QNODE_INIT {0,0}

typedef struct mcs_lock_qnode
{
	volatile struct mcs_lock_qnode* volatile next;
	volatile int locked;
} mcs_lock_qnode_t;

typedef struct mcs_lock
{
	mcs_lock_qnode_t* lock;
} mcs_lock_t;

typedef struct mcs_dissem_flags
{
	volatile int myflags[2][LOG2_MAX_VCORES];
	volatile int* partnerflags[2][LOG2_MAX_VCORES];
	int parity;
	int sense;
	char pad[ARCH_CL_SIZE];
} __attribute__((aligned(ARCH_CL_SIZE))) mcs_dissem_flags_t;

typedef struct mcs_barrier_t
{
	size_t nprocs;
	mcs_dissem_flags_t* allnodes;
	size_t logp;
} mcs_barrier_t;

#ifdef COMPILING_PARLIB
# define mcs_lock_init INTERNAL(mcs_lock_init)
# define mcs_lock_lock INTERNAL(mcs_lock_lock)
# define mcs_lock_unlock INTERNAL(mcs_lock_unlock)
# define mcs_barrier_init INTERNAL(mcs_barrier_init)
# define mcs_barrier_wait INTERNAL(mcs_barrier_wait)
#endif

void mcs_lock_init(struct mcs_lock *lock);
/* Caller needs to alloc (and zero) their own qnode to spin on.  The memory
 * should be on a cacheline that is 'per-thread'.  This could be on the stack,
 * in a thread control block, etc. */
void mcs_lock_lock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
void mcs_lock_unlock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
/* If you lock the lock from vcore context, you must use these. */
void mcs_lock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
void mcs_unlock_notifsafe(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);

void mcs_barrier_init(mcs_barrier_t* b, size_t nprocs);
void mcs_barrier_wait(mcs_barrier_t* b, size_t vcoreid);

#ifdef __cplusplus
}
#endif

#endif
