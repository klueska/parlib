/*
 * Copyright (c) 2011 The Regents of the University of California
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

#include <stddef.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "internal/vcore.h"
#include "internal/tls.h"
#include "internal/assert.h"
#include "internal/futex.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"

/* Reference to the main thread's tls descriptor */
void *main_tls_desc = NULL;

/* Current tls_desc for each running vcore, used when swapping contexts onto a vcore */
__thread void *current_tls_desc = NULL;

static __thread int tls_futex = 0;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void)
{
  /* This code has gone through many iterations of trying to create tls
   * regions and get them set up properly so that arbitrary user-level thread
   * code can make glibc calls safely.  After alot of headaches and trial and
   * error, I've finally settled on just creating an actual full blown pthread
   * with a minimal stack to get at a unique tls region that is properly
   * initialized and safe to make glibc calls using.  With this method, any
   * user level thread that gets created actually has a pthread backing it
   * from the perspective of the underlying system, but all we use from this
   * pthread is its properly initialized tls region.  The pthread itself,
   * simply spins up and parks itself on a futex until its tls region is no
   * longer needed. This slows things down on tls creation, but does not
   * affect any user-level scheduling that may go one above this. */
  void *start_routine(void *arg)
  {
    void **tcb = (void **)arg;
    *tcb = get_current_tls_base();
    futex_wakeup_one(tcb);

    futex_wait(&tls_futex, 0);
    return NULL;
  }

  void *tcb = NULL;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
  internal_pthread_create(&attr, start_routine, &tcb);
  futex_wait(&tcb, 0);
	return tcb;
}

/* Free a previously allocated TLS region */
void free_tls(void *tcb)
{
  begin_access_tls_vars(tcb);
  int *tls_futex_addr = &tls_futex;
  *tls_futex_addr = 1;
  futex_wakeup_one(tls_futex_addr);
  end_access_tls_vars();
}

/* Reinitialize / reset / refresh a TLS to its initial values.  This doesn't do
 * it properly yet, it merely frees and re-allocates the TLS, which is why we're
 * slightly ghetto and return the pointer you should use for the TCB. */
void *reinit_tls(void *tcb)
{
    free_tls(tcb);
    return allocate_tls();
}

/* Constructor to get a reference to the main thread's TLS descriptor */
int tls_lib_init()
{
	/* Make sure this only runs once */
	static bool initialized = false;
	if (initialized)
	    return 0;
	initialized = true;
	
	/* Get a reference to the main program's TLS descriptor */
	current_tls_desc = get_current_tls_base();
	main_tls_desc = current_tls_desc;
	return 0;
}

/* Initialize tls for use in this vcore */
void init_tls(uint32_t vcoreid)
{
	/* Get a reference to the current TLS region in the GDT */
	void *tcb = get_current_tls_base();
	assert(tcb);

  init_arch_tls_data(&__vcores[vcoreid].arch_tls_data, tcb, vcoreid);

	/* Set the tls_desc in the tls_desc array */
	__vcore_tls_descs[vcoreid] = tcb;
}

/* Passing in the vcoreid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t vcoreid)
{
  assert(tls_desc != NULL);

  set_current_tls_base(tls_desc, &__vcores[vcoreid].arch_tls_data);

  extern __thread int __vcore_id;
  current_tls_desc = tls_desc;
  __vcore_id = vcoreid;
}

/* Get the tls descriptor currently set for a given vcore. This should
 * only ever be called once the vcore has been initialized */
void *get_tls_desc(uint32_t vcoreid)
{
  void *desc = TLS_DESC(__vcores[vcoreid].arch_tls_data);
  assert(desc);
  return desc;
}

