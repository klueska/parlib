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
#include "internal/tls.h"
#include "internal/futex.h"
#include "context.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"
#include "mcs.h"

/* Array of vcores using clone to masquerade. */
struct vcore *__vcores = NULL;

/* Array of TLS descriptors to use for each vcore. Separate from the vcore
 * array so that we can access it transparently, as an array, outside of
 * the vcore library. */
void EXPORT_SYMBOL **vcore_tls_descs = NULL;

/**  Array of mappings from vcore id to pcore id */
int EXPORT_SYMBOL *vcore_map = NULL;

/* Id of the currently running vcore. */
__thread int EXPORT_SYMBOL __vcore_id = -1;

/* Stack of the currently running vcore. */
__thread void *__vcore_stack = NULL;

/* Per vcore contexts for entry at the top of the vcore stack */
__thread ucontext_t EXPORT_SYMBOL vcore_entry_context = { 0 };
__thread ucontext_t vcore_entry_gate_context = { 0 };

/* Current user context running on a vcore, this used to restore a user context
 * if it is interrupted for some reason without yielding voluntarily */
__thread ucontext_t EXPORT_SYMBOL *vcore_saved_ucontext = NULL;

/* Current tls_desc of the user context running on a vcore, this used to restore
 * a user's tls_desc if it is interrupted for some reason without yielding
 * voluntarily */
__thread void *vcore_saved_tls_desc = NULL;

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
static ucontext_t main_context = { 0 };
 
/* Generic stack allocation function using mmap */
void *__stack_alloc(size_t s)
{
  void *stack_bottom = mmap(0, s, PROT_READ|PROT_WRITE|PROT_EXEC,
                            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(stack_bottom == MAP_FAILED) {
    fprintf(stderr, "vcore: could not mmap stack!\n");
    exit(1);
  }
  return stack_bottom;
}

void __stack_free(void *stack, size_t s)
{
  munmap(stack, s);
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
  vcore_map[vcoreid] = cpuid;

  sched_yield();
}

/* Helper function used to reenter at the top of a vcore's stack for an
 * arbitrary function */
void vcore_reenter(void (*entry_func)(void))
{
  assert(in_vcore_context());
  __vcore_reenter(entry_func, __vcore_stack + VCORE_STACK_SIZE);
}

/* The entry gate of a vcore after it's initial creation. */
static void vcore_entry_gate()
{
  assert(__in_vcore_context);
  int vcoreid = __vcore_id;

  /* Update the vcore counts and set the flag for allocated to false */
  atomic_add(&__num_vcores, -1);
  atomic_set(&__vcores[vcoreid].allocated, false);

  /* Wait for this vcore to get woken up. */
  futex_wait(&__vcores[vcoreid].allocated, false);

  /* Vcore is awake. Jump to the vcore's entry point */
  vcore_entry();

  /* We never exit a vcore ... we always park them instead. If we did we would
   * need to take care not to exit the main vcore (experience shows that exits
   * the application). */
  fprintf(stderr, "vcore: failed to invoke vcore_yield\n");
  exit(1);
}

static void __vcore_init_context(struct ucontext *context, void (*func)(void))
{
  if (__vcore_stack == NULL)
    __vcore_stack = __stack_alloc(VCORE_STACK_SIZE);
  parlib_getcontext(context);
  context->uc_stack.ss_sp = __vcore_stack;
  context->uc_stack.ss_size = VCORE_STACK_SIZE;
  context->uc_link = 0;
  parlib_makecontext(context, func, 0);
}

static void __vcore_init(int vcoreid)
{
  /* Set the affinity on this vcore */
  __set_affinity(vcoreid, vcoreid);

  /* Switch to that tls region */
  set_tls_desc(vcore_tls_descs[vcoreid], vcoreid);

  /* Set that we are in vcore context */
  __in_vcore_context = true;

  /* Assign the id to the tls variable */
  __vcore_id = vcoreid;

  /* Set up useful contexts that start at the top of the vcore stack. */
  __vcore_init_context(&vcore_entry_context, vcore_entry);
  __vcore_init_context(&vcore_entry_gate_context, vcore_entry_gate);
}

static void * __vcore_trampoline_entry(void *arg)
{
  uint32_t vcoreid = (uintptr_t)arg;

  /* Initialize the tls region to be used by this vcore */
  init_tls(get_current_tls_base(), vcoreid);

  /* Initialize the vcore */
  __vcore_init(vcoreid);

  /* Jump to the vcore_entry_gate() and wait to be allocated */
  parlib_setcontext(&vcore_entry_gate_context);

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

  if ((errno = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN)) != 0) {
    fprintf(stderr, "vcore: could not set stack size of underlying pthread\n");
    exit(1);
  }
  if ((errno = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    fprintf(stderr, "vcore: could not set stack size of underlying pthread\n");
    exit(1);
  }
  
  /* Up the vcore count counts and set the flag for allocated until we
   * get a chance to stop the thread and deallocate it in its entry gate */
  atomic_set(&__vcores[i].allocated, true);
  atomic_add(&__num_vcores, 1);

  /* Actually create the vcore's backing pthread. */
  internal_pthread_create(&attr, __vcore_trampoline_entry, (void *) (long) i);
}

static int __vcore_request(int requested)
{
  /* If there is no chance of waking up requested vcores, just bail out */
  long nvc;
  do {
    nvc = atomic_read(&__num_vcores);
    if(requested > (__max_vcores - nvc))
      return -1;
  } while (!atomic_cas(&__num_vcores, nvc, nvc + requested));

  /* Otherwise try and reserve requested vcores for waking up */
  int reserved = 0;
  while (1) {
    for (int i = 0; i < __max_vcores; i++) {
      if (atomic_read(&__vcores[i].allocated) == false) {
        if (atomic_swap(&__vcores[i].allocated, true) == false) {
          futex_wakeup_one(&__vcores[i].allocated);
          if (++reserved == requested)
            return 0;
        }
      }
    }
  }
}

/* If this is the first vcore requested, do something special */

static bool vcore0_init()
{
  init_once_racy(return false);

  volatile bool once = false;
  parlib_getcontext(&main_context);

  if (!once) {
    once = true;
    /* Update the vcore counts and set the flag for allocated */
    atomic_set(&__vcores[0].allocated, true);
    atomic_set(&__num_vcores, 1);

    /* Vcore is awake. Jump to the vcore's entry point at the top of the
     * vcore stack. */
    set_tls_desc(vcore_tls_descs[0], 0);
    vcore_saved_ucontext = &main_context;
    vcore_saved_tls_desc = main_tls_desc;
    vcore_reenter(vcore_entry);
  }

  return true;
}

int vcore_request(int requested)
{
  if (requested < 0)
    return -1;

  if (requested > 0)
    requested -= vcore0_init();

  if (requested == 0)
    return 0;

  return __vcore_request(requested);
}

void EXPORT_SYMBOL vcore_yield()
{
  /* Clear out the saved ht_saved_context and ht_saved_tls_desc variables */
  vcore_saved_ucontext = NULL;
  vcore_saved_tls_desc = NULL;

#ifndef PARLIB_NO_UTHREAD_TLS
  /* Restore the TLS associated with this vcore's context */
  set_tls_desc(vcore_tls_descs[__vcore_id], __vcore_id);
#endif
  /* Jump to the transition stack allocated on this vcore's underlying
   * stack. This is only used very quickly so we can run the setcontext code */
  parlib_setcontext(&vcore_entry_gate_context);
}

int vcore_lib_init()
{
  /* Make sure this only runs once */
  run_once(
    /* Make sure the tls subsystem is up and running */
    assert(!tls_lib_init());

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
    __vcores = malloc(sizeof(struct vcore) * __max_vcores);
    vcore_tls_descs = malloc(sizeof(uintptr_t) * __max_vcores);
    vcore_map = malloc(sizeof(int) * __max_vcores);

    if (__vcores == NULL || vcore_tls_descs == NULL || vcore_map == NULL) {
      fprintf(stderr, "vcore: failed to initialize vcores\n");
      exit(1);
    }

    /* Initialize the vcore_map to a sentinel value */
    for (int i=0; i < __max_vcores; i++)
      vcore_map[i] = VCORE_UNMAPPED;

    /* Create other vcores. */
    for (int i = 1; i < __max_vcores; i++) {
      __create_vcore(i);
    }

    /* Initialize zeroth vcore (i.e., us).  We do this last so that the
     * pthreads that correspond to the vcores are contiguous in GDB. */
    init_tls(allocate_tls(), 0);
    __vcore_init(0);
    set_tls_desc(main_tls_desc, 0);

    /* Wait until they have parked. */
    while (atomic_read(&__num_vcores) > 0)
      cpu_relax();
  )
  return 0;
}

pthread_t internal_pthread_create(pthread_attr_t *attr,
                                  void *(*start_routine) (void *), void *arg)
{
  pthread_t pthread;

  if (attr != NULL) {
    size_t stack_size;
    if (pthread_attr_getstacksize(attr, &stack_size) != 0)
      abort();
    /* PTHREAD_STACK_MIN is not actually always sufficient because
     * the static TLS, whose size is program-dependent, is allocated
     * at the top of the stack.  Determine a sufficient value. */
    if (stack_size == PTHREAD_STACK_MIN) {
      static size_t min_stack_size = 0;
      if (min_stack_size == 0) {
        min_stack_size = PTHREAD_STACK_MIN;
        /* We assume pthread_create can only fail due to stack shortage.
         * Make sure your other attributes are legit! */
        while (pthread_create(&pthread, attr, start_routine, arg) != 0) {
          min_stack_size *= 2;
          pthread_attr_setstacksize(attr, min_stack_size);
        }
        return pthread;
      }
      if (pthread_attr_setstacksize(attr, min_stack_size) != 0)
        abort();
    }
  }

  if (pthread_create(&pthread, attr, start_routine, arg) != 0)
    abort();
  return pthread;
}

/* Clear pending, and try to handle events that came in between a previous call
 * to handle_events() and the clearing of pending.  While it's not a big deal,
 * we'll loop in case we catch any.  Will break out of this once there are no
 * events, and we will have send pending to 0. 
 *
 * Note that this won't catch every race/case of an incoming event.  Future
 * events will get caught in pop_ros_tf() */
void clear_notif_pending(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Only call this if you know what you are doing. */
static inline void __enable_notifs(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Enables notifs, and deals with missed notifs by self notifying.  This should
 * be rare, so the syscall overhead isn't a big deal. */
void enable_notifs(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

void disable_notifs(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

#undef vcore_lib_init
#undef vcore_request
#undef vcore_reenter
#undef clear_notif_pending
#undef enable_notifs
#undef disable_notifs
EXPORT_ALIAS(INTERNAL(vcore_lib_init), vcore_lib_init)
EXPORT_ALIAS(INTERNAL(vcore_request), vcore_request)
EXPORT_ALIAS(INTERNAL(vcore_reenter), vcore_reenter)
EXPORT_ALIAS(INTERNAL(clear_notif_pending), clear_notif_pending)
EXPORT_ALIAS(INTERNAL(enable_notifs), enable_notifs)
EXPORT_ALIAS(INTERNAL(disable_notifs), disable_notifs)
