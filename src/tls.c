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

#define _GNU_SOURCE
#include <stddef.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <limits.h>
#include <sys/sysinfo.h>

#include "internal/parlib.h"
#include "internal/vcore.h"
#include "internal/futex.h"
#include "timing.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"

/* Reference to the main thread's tls descriptor */
void *main_tls_desc = NULL;

/* TLS variables used by the pthread backing each uthread. */
__thread struct backing_pthread __backing_pthread;

static void *__create_backing_thread(void *tls_addr)
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
   * affect any user-level scheduling that may go one above this. We also 
   * overload this thread to simulate async I/O on behalf of the uthread it is
   * backing. */
  void *start_routine(void *arg)
  {
    void **tcb = (void **)arg;
    if (tls_addr == NULL) {
      /* Grab a reference to our tls_base and set it in the argument. */
      *tcb = get_current_tls_base();
    } else {
      /* Set our tls base to the tls passed in and set it in the argument. */
      *tcb = tls_addr;
      set_current_tls_base(*tcb);
    }

    /* Set it up so we can run syscalls on behalf of the uthread we are
     * backing with this pthread. */
    __backing_pthread.futex = BACKING_THREAD_SLEEP;
    __backing_pthread.syscall = NULL;
    __backing_pthread.arg = NULL;

    /* Wakeup the caller of this function with the newly set tls via a weird
     * usage of a futex. */
    futex_wakeup_one(tcb);

    /* Process syscalls or sleep until we are told to exit. */
    while(1) {
      switch(__backing_pthread.futex) {
        case BACKING_THREAD_SYSCALL: {
          cpu_set_t c;
          CPU_ZERO(&c);
          CPU_SET(vcore_id(), &c);
          sched_setaffinity(0, sizeof(cpu_set_t), &c);
          __backing_pthread.futex = BACKING_THREAD_SLEEP;
          __backing_pthread.syscall(__backing_pthread.arg);
          break;
        }
        case BACKING_THREAD_SLEEP:
          futex_wait(&__backing_pthread.futex, BACKING_THREAD_SLEEP);
          break;
        case BACKING_THREAD_EXIT:
          goto exit;
      }
    }
    exit: return NULL;
  }

  void *tcb = NULL;
  internal_pthread_create(PTHREAD_STACK_MIN, start_routine, &tcb);
  futex_wait(&tcb, 0);
  return tcb;
}

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void)
{
  return __create_backing_thread(NULL);
}

/* We need a backing pthread for the main thread too so that syscalls can run
 * somehwere when it makes them.  Piggyback that on this call. */
void *get_main_tls()
{
  return __create_backing_thread(main_tls_desc);
}

/* Free a previously allocated TLS region */
void free_tls(void *tcb)
{
  int *futex = get_tls_addr(__backing_pthread.futex, tcb);
  *futex = BACKING_THREAD_EXIT;
  futex_wakeup_one(futex);
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
	main_tls_desc = get_current_tls_base();
	return 0;
}

/* Initialize tls for use in this vcore */
void init_tls(void *tcb, uint32_t vcoreid)
{
#ifdef arch_tls_data_t
  init_arch_tls_data(&(__vcores(vcoreid).arch_tls_data), tcb, vcoreid);
#endif
  vcore_tls_descs(vcoreid) = tcb;
}

/* Passing in the vcoreid, since it'll be in TLS of the caller */
void __set_tls_desc(void *tls_desc, uint32_t vcoreid)
{
  assert(tls_desc != NULL);

#ifdef arch_tls_data_t
  set_current_tls_base(tls_desc, &__vcores(vcoreid).arch_tls_data);
#else
  set_current_tls_base(tls_desc);
#endif

  extern __thread int __vcore_id;
  __vcore_id = vcoreid;
}

#undef __set_tls_desc
EXPORT_ALIAS(INTERNAL(__set_tls_desc), __set_tls_desc)
