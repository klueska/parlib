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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "internal/parlib.h"
#include "parlib.h"
#include "atomic.h"
#include "mcs.h"

// MCS locks
void mcs_lock_init(struct mcs_lock *lock)
{
	memset(lock, 0, sizeof(mcs_lock_t));
}

static inline mcs_lock_qnode_t *mcs_qnode_swap(mcs_lock_qnode_t **addr,
                                               mcs_lock_qnode_t *val)
{
	return (mcs_lock_qnode_t*)atomic_exchange_acq((long*)addr,(long)val);
}

void mcs_lock_lock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)
{
	qnode->next = 0;
	mcs_lock_qnode_t* predecessor = mcs_qnode_swap(&lock->lock,qnode);
	if(predecessor)
	{
		qnode->locked = 1;
		predecessor->next = qnode;
		while(qnode->locked)
			cpu_relax();
	}
}

void mcs_lock_unlock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode)
{
	if(qnode == NULL) return;
	if(qnode->next == 0)
	{
		mcs_lock_qnode_t* old_tail = mcs_qnode_swap(&lock->lock,0);
		if(old_tail == qnode)
			return;

		mcs_lock_qnode_t* usurper = mcs_qnode_swap(&lock->lock,old_tail);
		while(qnode->next == 0);
		if(usurper)
			usurper->next = qnode->next;
		else
			qnode->next->locked = 0;
	}
	else
		qnode->next->locked = 0;
}

void mcs_pdr_init(struct mcs_pdr_lock *pdr_lock)
{
	memset(pdr_lock, 0, sizeof(mcs_lock_t));
}

void EXPORT_SYMBOL mcs_pdr_lock(struct mcs_pdr_lock *pdr_lock,
                                struct mcs_lock_qnode *qnode)
{
	if (!in_vcore_context() && current_uthread)
		uth_disable_notifs();
	mcs_lock_lock((struct mcs_lock *)pdr_lock, qnode);
}

void mcs_pdr_unlock(struct mcs_pdr_lock *pdr_lock, struct mcs_lock_qnode *qnode)
{
	mcs_lock_unlock((struct mcs_lock *)pdr_lock, qnode);
	if (!in_vcore_context() && current_uthread)
		uth_enable_notifs();
}

// MCS dissemination barrier!
void mcs_barrier_init(mcs_barrier_t* b, size_t np)
{
	assert(np <= max_vcores());
	b->allnodes = parlib_aligned_alloc(ARCH_CL_SIZE,
	                  np*sizeof(mcs_dissem_flags_t));
	memset(b->allnodes,0,np*sizeof(mcs_dissem_flags_t));
	b->nprocs = np;

	b->logp = (np & (np-1)) != 0;
	while(np >>= 1)
		b->logp++;

	size_t i,k;
	for(i = 0; i < b->nprocs; i++)
	{
		b->allnodes[i].parity = 0;
		b->allnodes[i].sense = 1;

		for(k = 0; k < b->logp; k++)
		{
			size_t j = (i+(1<<k)) % b->nprocs;
			b->allnodes[i].partnerflags[0][k] = &b->allnodes[j].myflags[0][k];
			b->allnodes[i].partnerflags[1][k] = &b->allnodes[j].myflags[1][k];
		} 
	}
}

void mcs_barrier_wait(mcs_barrier_t* b, size_t pid)
{
	mcs_dissem_flags_t* localflags = &b->allnodes[pid];
	size_t i;
	for(i = 0; i < b->logp; i++)
	{
		*localflags->partnerflags[localflags->parity][i] = localflags->sense;
		while(localflags->myflags[localflags->parity][i] != localflags->sense);
	}
	if(localflags->parity)
		localflags->sense = 1-localflags->sense;
	localflags->parity = 1-localflags->parity;
}

#undef mcs_lock_init
#undef mcs_pdr_init
#undef mcs_lock_lock
#undef mcs_lock_unlock
//#undef mcs_pdr_lock
#undef mcs_pdr_unlock
#undef mcs_barrier_init
#undef mcs_barrier_wait
EXPORT_ALIAS(INTERNAL(mcs_lock_init), mcs_lock_init)
EXPORT_ALIAS(INTERNAL(mcs_pdr_init), mcs_pdr_init)
EXPORT_ALIAS(INTERNAL(mcs_lock_lock), mcs_lock_lock)
EXPORT_ALIAS(INTERNAL(mcs_lock_unlock), mcs_lock_unlock)
//EXPORT_ALIAS(INTERNAL(mcs_pdr_lock), mcs_pdr_lock)
EXPORT_ALIAS(INTERNAL(mcs_pdr_unlock), mcs_pdr_unlock)
EXPORT_ALIAS(INTERNAL(mcs_barrier_init), mcs_barrier_init)
EXPORT_ALIAS(INTERNAL(mcs_barrier_wait), mcs_barrier_wait)
