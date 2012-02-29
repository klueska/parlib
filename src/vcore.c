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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include "internal/vcore.h"
#include "internal/tls.h"
#include "internal/futex.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"
#include "mcs.h"

/* Array of vcores using clone to masquerade. */
struct vcore *__vcores = NULL;

/* Array of TLS descriptors to use for each vcore. Separate from the vcore
 * array so that we can access it transparently, as an array, outside of
 * the vcore library. */
void **vcore_tls_descs = NULL;

/* Id of the currently running vcore. */
__thread int __vcore_id = -1;

/* Per vcore context with its own stack and TLS region */
__thread ucontext_t vcore_context = { 0 };

/* Current user context running on a vcore, this used to restore a user context
 * if it is interrupted for some reason without yielding voluntarily */
__thread ucontext_t *vcore_saved_ucontext = NULL;

/* Current tls_desc of the user context running on a vcore, this used to restore
 * a user's tls_desc if it is interrupted for some reason without yielding
 * voluntarily */
__thread void *vcore_saved_tls_desc = NULL;

/* Delineates whether the current context running on the vcore is the vcore
 * context or not.
 * itself */
__thread bool __in_vcore_context = false;

/* Number of currently allocated vcores. */
volatile int __num_vcores = 0;

/* Max number of vcores this application has requested. */
volatile int __max_vcores = 0;

/* Limit on the number of vcoress that can be allocated. */
volatile int __limit_vcores = 0;

/* Global context associated with the main thread.  Used when swapping this
 * context over to vcore0 */
static ucontext_t main_context = { 0 };

/* Mutex used to provide mutually exclusive access to our globals. */
//static pthread_mutex_t __vcore_mutex = PTHREAD_MUTEX_INITIALIZER;
static mcs_lock_t __vcore_mutex = MCS_LOCK_INIT;

/* Stack space to use when a vcore yields without a stack. */
static __thread void *__vcore_stack = NULL;

/* Flag used to indicate if the original main thread has completely migrated
 * over to a vcore or not.  Required so as not to clobber the stack if
 * the "new" main thread is started on the vcore before the original one
 * is done with all of its stack operations */
static int original_main_done = false;
 
/* Default callback for vcore_entry() */
static void __vcore_entry()
{
       // Do nothing by default...
}
extern void vcore_entry() __attribute__ ((weak, alias ("__vcore_entry")));

void __vcore_entry_gate()
{
  assert(__in_vcore_context);
  /* Cache a copy of vcoreid so we don't lose track of it over system call
   * invocations in which TLS might change out from under us. */
  int vcoreid = __vcore_id;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&__vcore_mutex, &qnode);
  {
    /* Check and see if we should wait at the gate. */
    if (__max_vcores > __num_vcores) {
      printf("%d not waiting at gate\n", vcoreid);
      mcs_lock_unlock(&__vcore_mutex, &qnode);
      goto entry;
    }

    /* Update vcore counts. */
    __num_vcores--;
    __max_vcores--;

    /* Deallocate the thread. */
    __vcores[vcoreid].allocated = false;

    /* Signal that we are about to wait on the futex. */
    __vcores[vcoreid].running = false;
  }
  mcs_lock_unlock(&__vcore_mutex, &qnode);

  /* Wait for this vcore to get woken up. */
  futex_wait(&(__vcores[vcoreid].running), false);

  /* Hard thread is awake. */
entry:
  /* Wait for the original main to reach the point of not requiring its stack
   * anymore.  TODO: Think of a better way to this.  Required for now because
   * we need to call mcs_lock_unlock() in this thread, which causes a race
   * for using the main threads stack with the vcore restarting it in
   * ht_entry(). */
  while(!original_main_done) 
    cpu_relax();
  /* Use cached value of htid in case TLS changed out from under us while
   * waiting for this vcore to get woken up. */
  assert(__vcores[vcoreid].running == true);
  /* Jump to the vcore's entry point */
  vcore_entry();

  fprintf(stderr, "vcore: failed to invoke vcore_yield\n");

  /* We never exit a vcore ... we always park them and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main vcore (experience shows that exits the
   * application) and we should probably detach any created vcores
   * (but this may be debatable because of our implementation of
   * htls).
   */
  exit(1);
}

#ifdef VCORE_USE_PTHREAD
void *
#else
int 
#endif
__vcore_trampoline_entry(void *arg)
{
  assert(sizeof(void *) == sizeof(long int));

  int vcoreid = (int) (long int) arg;

#ifndef VCORE_USE_PTHREAD
  /* Set the proper affinity for this vcore */
  cpu_set_t c;
  CPU_ZERO(&c);
  CPU_SET(htid, &c);
  if((sched_setaffinity(0, sizeof(cpu_set_t), &c)) != 0) {
    fprintf(stderr, "vcore: could not set affinity of underlying pthread\n");
    exit(1);
  }
  sched_yield();
#endif

  /* Initialize the tls region to be used by this ht */
  init_tls(vcoreid);
  set_tls_desc(vcore_tls_descs[vcoreid], vcoreid);

  /* Assign the id to the tls variable */
  __vcore_id = vcoreid;

  /* Set it that we are in ht context */
  __in_vcore_context = true;

  /* If this is the first ht, set ht_saved_ucontext to the main_context in this
   * guys TLS region. MUST be done here, so in proper TLS */
  static int once = true;
  if(once) {
    once = false;
    vcore_saved_ucontext = &main_context;
    vcore_saved_tls_desc = main_tls_desc;
  }

  /*
   * We create stack space for the function 'setcontext' jumped to
   * after an invocation of ht_yield.
   */
  if ((__vcore_stack = alloca(getpagesize())) == NULL) {
    fprintf(stderr, "vcore: could not allocate space on the stack\n");
    exit(1);
  }

  __vcore_stack = (void *)(((size_t) __vcore_stack) + getpagesize());

  /*
   * We need to save a context because experience shows that we seg fault if we
   * clean up this vcore (i.e. call exit) on a stack other than the
   * original stack (maybe because certain things are placed on the stack like
   * cleanup functions).
   */
  if (getcontext(&vcore_context) < 0) {
    fprintf(stderr, "vcore: could not get context\n");
    exit(1);
  }

  if ((vcore_context.uc_stack.ss_sp = alloca(getpagesize())) == NULL) {
    fprintf(stderr, "vcore: could not allocate space on the stack\n");
    exit(1);
  }

  vcore_context.uc_stack.ss_size = getpagesize();
  vcore_context.uc_link = 0;

  makecontext(&vcore_context, (void (*) ()) __vcore_entry_gate, 0);
  setcontext(&vcore_context);

  /* We never exit a vcore ... we always park them and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main thread (experience shows that exits the
   * application) and we should probably detach any created vcores
   * (but this may be debatable because of our implementation of
   * htls).
   */
  fprintf(stderr, "vcore: failed to invoke vcore_yield\n");
  exit(1);
}

static void __create_vcore(int i)
{
#ifdef VCORE_USE_PTHREAD
  struct vcore *cht = &__vcores[i];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  cpu_set_t c;
  CPU_ZERO(&c);
  CPU_SET(i, &c);
  if ((errno = pthread_attr_setaffinity_np(&attr,
                                    sizeof(cpu_set_t),
                                    &c)) != 0) {
    fprintf(stderr, "vcore: could not set affinity of underlying pthread\n");
    exit(1);
  }
  if ((errno = pthread_attr_setstacksize(&attr, 4*PTHREAD_STACK_MIN)) != 0) {
    fprintf(stderr, "vcore: could not set stack size of underlying pthread\n");
    exit(1);
  }
  if ((errno = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    fprintf(stderr, "vcore: could not set stack size of underlying pthread\n");
    exit(1);
  }
  
  /* Set the created flag for the thread we are about to spawn off */
  cht->created = true;

  /* Also up the thread counts and set the flags for allocated and running
   * until we get a chance to stop the thread and deallocate it in its entry
   * gate */
  cht->allocated = true;
  cht->running = true;
  __num_vcores++;
  __max_vcores++;

  if ((errno = pthread_create(&cht->thread,
                              &attr,
                              __vcore_trampoline_entry,
                              (void *) (long int) i)) != 0) {
    fprintf(stderr, "vcore: could not allocate underlying vcore\n");
    exit(2);
  }
#else 
  struct vcore *cvcore = &__vcores[i];
  cvcore->stack_size = VCORE_MIN_STACK_SIZE;
  if((cvcore->stack_top = malloc(cvcore->stack_size)) == NULL) {
    fprintf(stderr, "vcore: could not set stack size of underlying vcore\n");
    exit(1);
  }
  if((cvcore->tls_desc = allocate_tls()) == NULL) {
    fprintf(stderr, "vcore: could not allocate tls for underlying vcore\n");
    exit(1);
  }
  init_tls(i);
  cvcore->ldt_entry.entry_number = 6;
  cvcore->ldt_entry.base_addr = (uintptr_t)cvcore->tls_desc;
  
  /* Set the created flag for the thread we are about to spawn off */
  cvcore->created = true;

  /* Also up the thread counts and set the flags for allocated and running
   * until we get a chance to stop the thread and deallocate it in its entry
   * gate */
  cvcore->allocated = true;
  cvcore->running = true;
  __num_vcores++;
  __max_vcores++;

  int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES 
                     | CLONE_SIGHAND | CLONE_THREAD
                     | CLONE_SETTLS | CLONE_PARENT_SETTID
                     | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
                     | CLONE_DETACHED
                     | 0);

  if(clone(__vcore_trampoline_entry, cvcore->stack_top+cvcore->stack_size, 
           clone_flags, (void *)((long int)i), 
           &cvcore->ptid, &cvcore->ldt_entry, &cvcore->ptid) == -1) {
    perror("Error");
    exit(2);
  }
#endif
}

static int __vcore_allocate(int k)
{
  int j = 0;
  for (; k > 0; k--) {
    for (int i = 0; i < __limit_vcores; i++) {
      assert(__vcores[i].created);
      if (!__vcores[i].allocated) {
        assert(__vcores[i].running == false);
        __vcores[i].allocated = true;
        __vcores[i].running = true;
        futex_wakeup_one(&(__vcores[i].running));
        j++;
        break;
      }
    }
  }
  return j;
}

static int __vcore_request(int k)
{
  /* Short circuit if k == 0 */
  if(k == 0)
    return 0;

  /* Determine how many vcores we can allocate. */
  int available = __limit_vcores - __max_vcores;
  k = available >= k ? k : available;

  /* Update vcore counts. */
  __max_vcores += k;

  /* Allocate as many as known available. */
  k = __max_vcores - __num_vcores;
  int j = __vcore_allocate(k);
  if(k != j) {
    printf("&k, %p, k: %d\n", &k, k);
  }
  fflush(stdout);
  assert(k == j);

  /* Update vcore counts. */
  __num_vcores += j;

  return k;
}

int vcore_request(int k)
{
  if(!__limit_vcores) {
    fprintf(stderr, "vcore: __limit_vcores == 0: Are you sure you called vcore_lib_init()?\n");
    exit(1);
  }
 
  int vcores = 0;
  // Always access the qnode through the qnode_ptr variable.  The reasons are
  // made clearer below. In the normal path we will just acquire and release
  // using this qnode.
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&__vcore_mutex, &qnode);
  {
    if (k == 0) {
      mcs_lock_unlock(&__vcore_mutex, &qnode);
      return 0;
    } else {
      /* If this is the first vcore requested, do something special */
      static int once = true;
      if(once) {
        getcontext(&main_context);
        cmb();
        if(once) {
          once = false;
          __vcore_request(1);
          /* Don't use the stack anymore! */
		  /* This is also my poor man's lock to ensure the main context is not
		   * restored until after I'm done using the stack and this "version"
		   * of the main thread goes to sleep forever */
          original_main_done = true;
          /* Futex calls are forced inline */
          futex_wait(&original_main_done, true);
          assert(0);
        }
        vcores = 1;
        k -=1;
      }
      /* Put in the rest of the request as normal */
      vcores += __vcore_request(k);
    }
  }
  mcs_lock_unlock(&__vcore_mutex, &qnode);
  return 0;
}

void vcore_yield()
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
  set_stack_pointer(__vcore_stack);
  /* Go back to the ht entry gate */
  setcontext(&vcore_context);
}

int vcore_lib_init()
{
  /* Make sure this only runs once */
  static bool initialized = false;
  if (initialized)
      return -1;
  initialized = true;

  /* Initialize the tls subsystem */
  assert(!tls_lib_init());

  /* Get the number of available vcores in the system */
  char *limit = getenv("VCORE_LIMIT");
  if (limit != NULL) {
    __limit_vcores = atoi(limit);
  } else {
    __limit_vcores = get_nprocs();  
  }

  /* Allocate the structs containing meta data about the vcores
   * themselves. Never freed though.  Just freed automatically when the program
   * dies since vcores should be alive for the entire lifetime of the
   * program. */
  __vcores = malloc(sizeof(struct vcore) * __limit_vcores);
  vcore_tls_descs = malloc(sizeof(uintptr_t) * __limit_vcores);

  if (__vcores == NULL || vcore_tls_descs == NULL) {
    fprintf(stderr, "vcore: failed to initialize vcores\n");
    exit(1);
  }

  /* Initialize. */
  for (int i = 0; i < __limit_vcores; i++) {
    __vcores[i].created = true;
    __vcores[i].allocated = true;
    __vcores[i].running = true;
  }

  /* Create all the vcores up front */
  for (int i = 0; i < __limit_vcores; i++) {
    /* Create all the vcores */
    __create_vcore(i);

	/* Make sure the threads have all started and are ready to be allocated
     * before moving on */
    while (__vcores[i].running == true)
      cpu_relax();
  }
  return 0;
}

/* Clear pending, and try to handle events that came in between a previous call
 * to handle_events() and the clearing of pending.  While it's not a big deal,
 * we'll loop in case we catch any.  Will break out of this once there are no
 * events, and we will have send pending to 0. 
 *
 * Note that this won't catch every race/case of an incoming event.  Future
 * events will get caught in pop_ros_tf() */
void clear_notif_pending(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Only call this if you know what you are doing. */
static inline void __enable_notifs(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Enables notifs, and deals with missed notifs by self notifying.  This should
 * be rare, so the syscall overhead isn't a big deal. */
void enable_notifs(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

void disable_notifs(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

