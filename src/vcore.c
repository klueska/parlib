/*
 * Copyright (c) 2011 The Regents of the University of California
 * Benjamin Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 *
 * This file is part of Parlib.
 *
 * The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * Linux vcore implementation
 */

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#define _GNU_SOURCE

#include "internal/parlib.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>

#include "parlib.h"
#include "internal/vcore.h"
#include "internal/futex.h"
#include "context.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"
#include "mcs.h"
#include "event.h"

/* Per vcore data */
struct vcore_pvc_data EXPORT_SYMBOL *vcore_pvc_data;
struct internal_vcore_pvc_data *internal_vcore_pvc_data;

/* Id of the currently running vcore. */
__thread int EXPORT_SYMBOL __vcore_id = -1;

/* Top of stack of the currently running vcore. */
static __thread void *__vcore_stack = NULL;

/* A pointer to the current signal stack in use by the vcore */
__thread void *__vcore_sigstack = NULL;

/* Current user context running on a vcore, this used to restore a user context
 * if it is interrupted for some reason without yielding voluntarily */
__thread struct user_context EXPORT_SYMBOL *vcore_saved_ucontext = NULL;

/* Current tls_desc of the user context running on a vcore, this used to restore
 * a user's tls_desc if it is interrupted for some reason without yielding
 * voluntarily */
__thread void EXPORT_SYMBOL *vcore_saved_tls_desc = NULL;

/* Delineates whether the current context running on the vcore is the vcore
 * context or not. Default is false, so we don't have to set it whenever we
 * create new contexts. */
__thread bool EXPORT_SYMBOL __in_vcore_context = false;

/* Number of currently allocated vcores. */
atomic_t EXPORT_SYMBOL __num_vcores = ATOMIC_INITIALIZER(0);

/* Maximum number of vcores that can ever be allocated. */
volatile int EXPORT_SYMBOL __max_vcores = 0;

/* Global context associated with the main thread.  Used when swapping this
 * context over to vcore0 */
static struct user_context main_context = { 0 };

/* Global constants indicating the size and alignment of the static TLS
 * region. Needs to be initialized at run time and done in vcore_lib_init(). */
static size_t __static_tls_size = -1;
static size_t __static_tls_align = -1;
 
/* Minimum possible stack size.  Needs to be set based on the
 * __static_tls_size at runtime. */
static size_t __min_stack_size = -1;

/* Allocate a new signal stack to the vcore and save a pointer to the old one
 * in the variable passed in. If the pointer passed in is NULL, then it is not
 * assigned.  */
void __sigstack_swap(void **sigstack)
{
	if (sigstack) {
		__sigstack_free(*sigstack);
		*sigstack = __vcore_sigstack;
	}
	__vcore_sigstack = parlib_aligned_alloc(PGSIZE, SIGSTKSZ);

	stack_t altstack;
	altstack.ss_sp = __vcore_sigstack;
	altstack.ss_size = SIGSTKSZ;
	altstack.ss_flags = 0;
	assert(sigaltstack(&altstack, NULL) == 0);
}

/* Free the signal stack */
void __sigstack_free(void *sigstack)
{
	free(sigstack);
}

/* Generic set affinity function */
static void __set_affinity(int vcoreid, int cpuid)
{
  /* Set the proper affinity for this vcore */
  /* Moved here so that we are assured we are fully on the vcore before doing
   * any substantial work that makes any glibc library calls (like calling
   * set_affinity(), which may call malloc underneath). */
  cpu_set_t c;
  CPU_ZERO(&c);
  CPU_SET(cpuid, &c);
  if((sched_setaffinity(0, sizeof(cpu_set_t), &c)) != 0)
    fprintf(stderr, "vcore: could not set affinity of underlying pthread\n");

  /* Set the entry in the vcore_map to the cpuid */
  vcore_map(vcoreid) = cpuid;

  sched_yield();
}

/* Wrapper function for the entry function from a vcore signal */
static void __vcore_sigentry(int sig, siginfo_t *info, void *context)
{
	assert(sig == SIGVCORE);

	/* If I'm able to successfully do a vcore_request_specific(), then the
	 * vcore this signal is destined for must have been offline. It will now
	 * come back online shortly, and trigger the vcore_sigentry() then. */
	if (vcore_request_specific(__vcore_id) == 0)
		return;
	/* Otherwise, if I'm in vcore context, just set vcore_sigpending and it
	 * will be processed when next appropriate. */
	if (in_vcore_context()) {
		atomic_set(&__vcore_sigpending(__vcore_id), 1);
		return;
	}
	/* Otherwise, just call out to the generic vcore_sigentry() function. */
	vcore_sigentry();
}

/* Generic sigaction function to set up a singal handler for sending a signal
 * to a vcore */
static void __set_sigaction()
{
	struct sigaction act;
	act.sa_sigaction = __vcore_sigentry;
	act.sa_flags = SA_SIGINFO;
	sigemptyset(&act.sa_mask);
	sigaction(SIGVCORE, &act, NULL);
}

/* Function for sending a signal to a vcore. */
void EXPORT_SYMBOL vcore_signal(int vcoreid) {
  if (!__vcore_sigpending(vcoreid))
	  pthread_kill(__vcores(vcoreid).pthread, SIGVCORE);
}

/* Helper function used to reenter at the top of a vcore's stack for an
 * arbitrary function */
void vcore_reenter(void (*entry_func)(void))
{
  assert(in_vcore_context());
  __vcore_reenter(entry_func, __vcore_stack);
}

/* The entry gate of a vcore after it's initial creation. */
static void vcore_entry_gate()
{
  assert(__in_vcore_context);
  int vcoreid = __vcore_id;

  /* Deallocate the vcore. */
  atomic_set(&__vcores(vcoreid).allocated, false);
  atomic_add(&__num_vcores, -1);

  /* Rerequest the vcore if a signal is pending. This has to come after
   * deallocating the vcore above. Doing so allows us to never miss a signal
   * because of the synchronization present in the __vcore_sigentry() call. */
  if (atomic_swap(&__vcore_sigpending(vcoreid), 0) == 1)
    vcore_request_specific(vcoreid);

  /* Wait for this vcore to get woken up. */
  futex_wait(&__vcores(vcoreid).allocated, false);
  atomic_add(&__num_vcores, 1);

  /* Vcore is awake. Jump to the vcore's entry point */
  vcore_entry();

  /* We never exit a vcore ... we always park them instead. If we did we would
   * need to take care not to exit the main vcore (experience shows that exits
   * the application). */
  fprintf(stderr, "vcore: failed to invoke vcore_yield\n");
  exit(1);
}

static void *get_stack_top()
{
  size_t np_stack_size;
  void *stack_limit;
  pthread_attr_t np_attr_stack;

  /* Get the stack and its size. */
  if (pthread_attr_init(&np_attr_stack))
    abort();
  if (pthread_getattr_np(pthread_self(), &np_attr_stack))
    abort();
  if (pthread_attr_getstack(&np_attr_stack, &stack_limit, &np_stack_size))
    abort();

  /* Randomly offset the stacks slightly so they don't interfere with one
   * another if they end up running in lock step. */
  int offset = __vcore_id * ARCH_CL_SIZE;

  /* Account for the TLS sitting at the top of the stack. */
  return stack_limit + np_stack_size - __static_tls_size - offset;
}

static void __vcore_init(int vcoreid)
{
  /* Set the affinity on this vcore */
  __set_affinity(vcoreid, vcoreid);

  /* Switch to the proper tls region */
  __set_tls_desc(vcore_tls_descs(vcoreid), vcoreid);

  /* Set the signal stack for this vcore */
  __sigstack_swap(NULL);

  /* Set that we are in vcore context */
  __in_vcore_context = true;

  /* Assign the id to the tls variable */
  __vcore_id = vcoreid;

  /* Store a pointer to the backing pthread for this vcore */
  __vcores(vcoreid).pthread = pthread_self();

  /* Determine top of vcore stack */
  __vcore_stack = get_stack_top();
}

static void * __vcore_trampoline_entry(void *arg)
{
  uint32_t vcoreid = (uintptr_t)arg;

  /* Initialize the tls region to be used by this vcore */
  init_tls(get_current_tls_base(), vcoreid);

  /* Initialize the vcore */
  __vcore_init(vcoreid);

  /* Jump to the vcore_entry_gate() and wait to be allocated */
  vcore_reenter(vcore_entry_gate);

  /* We never exit a vcore ... we always park them and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main thread (experience shows that exits the
   * application) and we should probably detach any created vcores
   * (but this may be debatable because of our implementation of
   * htls).
   */
  fprintf(stderr, "vcore: something is really screwed up if we get here!\n");
  exit(1);
}

static void __create_vcore(int i)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  /* Up the vcore count counts and set the flag for allocated until we
   * get a chance to stop the thread and deallocate it in its entry gate */
  atomic_add(&__num_vcores, 1);
  atomic_set(&__vcores(i).allocated, true);
  atomic_set(&__vcores(i).reserved, false);

  /* Actually create the vcore's backing pthread. */
  internal_pthread_create(VCORE_STACK_SIZE, __vcore_trampoline_entry, (void*)(long)i);
}

/* If this is the first vcore requested, do something special */
static bool vcore_request_init()
{
  init_once_racy(return false);

  volatile bool once = false;
  parlib_getcontext(&main_context);

  if (!once) {
    once = true;
    __set_tls_desc(vcore_tls_descs(0), 0);
    vcore_saved_ucontext = &main_context;
    vcore_saved_tls_desc = main_tls_desc;

    void cb()
    {
      int sleepforever = true;
      vcore_request_specific(0);
      while(1) futex_wait(&sleepforever, true);
    }
    void *stack = malloc(PGSIZE);
    __vcore_reenter(cb, stack + PGSIZE);
  }

  return true;
}

static bool vcore_reserve_specific(int vcoreid)
{
  if (atomic_read(&__vcores(vcoreid).allocated) == false) {
    if (atomic_swap(&__vcores(vcoreid).reserved, true) == false) {
      return true;
    }
  }
  return false;
}

static bool reserve_vcores(int count, bool *list)
{
  /* Try and reserve exactly 'count' vcores. If we can, return true. */
  int reserved = 0;
  for (int i=0; i < max_vcores(); i++) {
    if (vcore_reserve_specific(i)) {
       list[i] = true;
       if (++reserved == count)
        return true;
    }
  }

  /* If we weren't able to reserve all 'count' vcores, then cancel the ones we
   * were able to reserve, and return false. */
  for (int i=0; reserved && (i < max_vcores()); i++) {
    if (list[i]) {
      atomic_set(&__vcores(i).reserved, false);
      reserved--;
    }
  }
  return false;
}

int vcore_request_specific(int vcoreid)
{
  if (vcore_reserve_specific(vcoreid)) {
    atomic_set(&__vcores(vcoreid).reserved, false);
    atomic_set(&__vcores(vcoreid).allocated, true);
    futex_wakeup_one(&__vcores(vcoreid).allocated);
    return 0;
  }
  return -1;
}

static int __vcore_request(int requested)
{
  /* If we aren't able to reserve all of the requested vcores for allocation,
   * then we fail without allocating any of them.. */
  bool reserved[max_vcores()];
  if (!reserve_vcores(requested, reserved))
    return -1;

  /* Otherwise wake up exactly the number of vcores we just reserved, and
   * return success. */
  for (int i = 0; i < __max_vcores; i++) {
    if (reserved[i]) {
      atomic_set(&__vcores(i).reserved, false);
      atomic_set(&__vcores(i).allocated, true);
      futex_wakeup_one(&__vcores(i).allocated);
    }
  }
  return 0;
}

int vcore_request(int requested)
{
  if (requested < 0)
    return -1;

  if (requested > 0)
    requested -= vcore_request_init();

  if (requested == 0)
    return 0;

  return __vcore_request(requested);
}

void EXPORT_SYMBOL vcore_yield()
{
#ifndef PARLIB_NO_UTHREAD_TLS
  /* Restore the TLS associated with this vcore's context */
  set_tls_desc(vcore_tls_descs(__vcore_id));
#endif
  /* Jump to the transition stack allocated on this vcore's underlying
   * stack. This is only used very quickly so we can run the setcontext code */
  vcore_reenter(vcore_entry_gate);
}

int vcore_lib_init()
{
  /* Make sure this only runs once */
  run_once(
    /* Make sure the tls subsystem is up and running */
    assert(!tls_lib_init());

	/* Get the static tls size and alignment, and calculate min stack size */
    extern void _dl_get_tls_static_info(size_t*, size_t*) internal_function;
    _dl_get_tls_static_info(&__static_tls_size, &__static_tls_align);
    __min_stack_size = PTHREAD_STACK_MIN + __static_tls_size;

    /* Get the number of available vcores in the system */
    char *limit = getenv("VCORE_LIMIT");
    if (limit != NULL) {
      __max_vcores = atoi(limit);
    } else {
      __max_vcores = get_nprocs();
    }

    /* Allocate the structs containing meta data about the vcores
     * themselves. Never freed though.  Just freed automatically when the program
     * dies since vcores should be alive for the entire lifetime of the
     * program. */
    vcore_pvc_data = parlib_aligned_alloc(PGSIZE,
                         sizeof(struct vcore_pvc_data) * __max_vcores);
    internal_vcore_pvc_data = parlib_aligned_alloc(PGSIZE,
                         sizeof(struct internal_vcore_pvc_data) * __max_vcores);

    if (vcore_pvc_data == NULL) {
      fprintf(stderr, "vcore: failed to initialize vcores\n");
      exit(1);
    }

    /* Initialize the vcore_sigpending array */
    for (int i=0; i<max_vcores(); i++)
      __vcore_sigpending(i) = ATOMIC_INITIALIZER(0);

    /* Initialize the vcore_map to a sentinel value */
    for (int i=0; i < __max_vcores; i++)
      vcore_map(i) = VCORE_UNMAPPED;

    /* Set the hignal handler for signals sent to all vcores (inherited) */
    __set_sigaction();

    /* Create all the vcores. */
    /* Previously, we reused the main thread for vcore 0, but this causes
     * problems with signaling for vcore 0, so now we just create them all the
     * same way and leae the main thread alone */
    for (int i = 0; i < __max_vcores; i++) {
      __create_vcore(i);
    }

    /* Initialize the event subsystem */
    event_lib_init();

    /* Wait until they have parked. */
    while (atomic_read(&__num_vcores) > 0)
      cpu_relax();
  )
  return 0;
}

/* An internal version of pthread create that always takes a stack size and 
 * returns the pthread created.  All stack sizes are adjusted to make sure
 * that pthread_create will not fail, and all threads are launched in the
 * detached state.  This function call cannot fail. */
pthread_t internal_pthread_create(size_t stack_size,
                                  void *(*start_routine) (void *), void *arg)
{
  pthread_t pthread;
  pthread_attr_t attr;

  /* Increase our stack to at least the minimum so we don't fail. */
  if (stack_size < __min_stack_size)
    stack_size = __min_stack_size;

  /* Set the pthread attributes */
  pthread_attr_init(&attr);
  if ((errno = pthread_attr_setstacksize(&attr, stack_size)) != 0) {
    fprintf(stderr, "Could not set stack size of underlying pthread\n");
    exit(1);
  }
  if ((errno = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    fprintf(stderr, "Could not set underlying pthread to detached state\n");
    exit(1);
  }
  /* Create the pthread */
  if ((errno = pthread_create(&pthread, &attr, start_routine, arg)) != 0) {
    fprintf(stderr, "Could not create pthread\n");
    exit(1);
  }
  return pthread;
}

#undef vcore_lib_init
#undef vcore_request
#undef vcore_request_specific
#undef vcore_reenter
EXPORT_ALIAS(INTERNAL(vcore_lib_init), vcore_lib_init)
EXPORT_ALIAS(INTERNAL(vcore_request), vcore_request)
EXPORT_ALIAS(INTERNAL(vcore_request_specific), vcore_request_specific)
EXPORT_ALIAS(INTERNAL(vcore_reenter), vcore_reenter)
