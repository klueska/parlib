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

#include "internal/vcore.h"
#include "internal/tls.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"

/* The current dymamic tls implementation currently uses a locked linked list
 * to find the key for a given thread. We should probably find a better way to
 * do this based on a custom lock-free hash table or something. */
#include <parlib/queue.h>
#include <parlib/spinlock.h>

/* The dynamic tls key structure */
struct dtls_key {
  spinlock_t lock;
  int ref_count;
  bool valid;
  void (*dtor)(void*);
};

/* The definition of a dtls_key list and its elements */
typedef struct dtls_list_element {
  TAILQ_ENTRY(dtls_list_element) link;
  dtls_key_t *key;
  void *dtls;
} dtls_list_element_t;
TAILQ_HEAD(dtls_list, dtls_list_element);

/* Reference to the main thread's tls descriptor */
void *main_tls_desc = NULL;

/* Current tls_desc for each running vcore, used when swapping contexts onto a vcore */
__thread void *current_tls_desc = NULL;

/* A pointer to a list of dynamically allocatable tls regions */
__thread dtls_list_t *current_dtls_list;

/* The list of dynamically allocatable tls regions itself */
__thread dtls_list_t __dtls_list;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void)
{
	extern void *_dl_allocate_tls(void *mem) internal_function;
	void *tcb = _dl_allocate_tls(NULL);
	if (!tcb)
		return 0;
	/* Make sure the TLS is set up properly - its tcb pointer 
	 * points to itself. */
	tcbhead_t *head = (tcbhead_t*)tcb;
	size_t offset = offsetof(tcbhead_t, multiple_threads);

	/* These fields in the tls_desc need to be set up for linux to work
	 * properly with TLS. Take a look at how things are done in libc in the
	 * nptl code for reference. */
	memcpy(tcb+offset, main_tls_desc+offset, sizeof(tcbhead_t)-offset);
	head->tcb = tcb;
	head->self = tcb;
	head->multiple_threads = true;
	return tcb;
}

/* Reinitialize / reset / refresh a TLS to its initial values.  This doesn't do
 * it properly yet, it merely frees and re-allocates the TLS, which is why we're
 * slightly ghetto and return the pointer you should use for the TCB. */
void *reinit_tls(void *tcb)
{
    free_tls(tcb);
    return allocate_tls();
}

/* Free a previously allocated TLS region */
void free_tls(void *tcb)
{
	extern void _dl_deallocate_tls (void *tcb, bool dealloc_tcb) internal_function;
	assert(tcb);
	_dl_deallocate_tls(tcb, 1);
}

/* Constructor to get a reference to the main thread's TLS descriptor */
int tls_lib_init()
{
	/* Make sure this only runs once */
	static bool initialized = false;
	if (initialized)
	    return -1;
	initialized = true;
	
	/* Get a reference to the main program's TLS descriptor */
	current_tls_desc = get_current_tls_base();
	main_tls_desc = current_tls_desc;
	return 0;
}

/* Default callback after tls constructor has finished */
static void __tls_ready()
{
	// Do nothing by default...
}
extern void tls_ready() __attribute__ ((weak, alias ("__tls_ready")));

/* Initialize tls for use in this vcore */
void init_tls(uint32_t vcoreid)
{
	/* Get a reference to the current TLS region in the GDT */
	void *tcb = get_current_tls_base();
	assert(tcb);

#ifdef __i386__
	/* Set up the TLS region as an entry in the LDT */
	struct user_desc *ud = &(__vcores[vcoreid].ldt_entry);
	memset(ud, 0, sizeof(struct user_desc));
	ud->entry_number = vcoreid + RESERVED_LDT_ENTRIES;
	ud->limit = (-1);
	ud->seg_32bit = 1;
	ud->limit_in_pages = 1;
	ud->useable = 1;
#elif __x86_64__
	__vcores[vcoreid].current_tls_base = tcb;
#endif

	/* Set the tls_desc in the tls_desc array */
	vcore_tls_descs[vcoreid] = tcb;
}

/* Passing in the vcoreid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t vcoreid)
{
  assert(tls_desc != NULL);

#if __i386__
  struct user_desc *ud = &(__vcores[vcoreid].ldt_entry);
  ud->base_addr = (unsigned int)tls_desc;
  int ret = syscall(SYS_modify_ldt, 1, ud, sizeof(struct user_desc));
  assert(ret == 0);
  TLS_SET_SEGMENT_REGISTER(ud->entry_number, 1);
#elif __x86_64__
  __vcores[vcoreid].current_tls_base = tls_desc;
  arch_prctl(ARCH_SET_FS, tls_desc);
#endif

  extern __thread int __vcore_id;
  current_tls_desc = tls_desc;
  __vcore_id = vcoreid;
}

/* Get the tls descriptor currently set for a given vcore. This should
 * only ever be called once the vcore has been initialized */
void *get_tls_desc(uint32_t vcoreid)
{
#if __i386__
	struct user_desc *ud = &(__vcores[vcoreid].ldt_entry);
	assert(ud->base_addr != 0);
	return (void *)(unsigned long)ud->base_addr;
#elif __x86_64__
	assert(__vcores[vcoreid].current_tls_base != 0);
	return __vcores[vcoreid].current_tls_base;
#endif
}

/* Make sure we use the libc versions of these calls, and not any custom ones.
 * Eventually I want to move away from using malloc at all, and pull from a
 * statically allocated pool. */
extern typeof(malloc) __libc_malloc;
extern typeof(free) __libc_free;

dtls_key_t *dtls_key_create(dtls_dtor_t dtor)
{
  dtls_key_t *key = __libc_malloc(sizeof(dtls_key_t));
  assert(key);

  spinlock_init(&key->lock);
  key->ref_count = 1;
  key->valid = true;
  key->dtor = dtor;
  return key;
}

void dtls_key_delete(dtls_key_t *key)
{
  assert(key);

  spinlock_lock(&key->lock);
  key->valid = false;
  key->ref_count--;
  spinlock_unlock(&key->lock);
  if(key->ref_count == 0)
    __libc_free(key);
}

void set_dtls(dtls_key_t *key, void *dtls)
{
  assert(key);
  if(current_dtls_list == NULL) {
    current_dtls_list = &__dtls_list;
    TAILQ_INIT(current_dtls_list);
  }

  spinlock_lock(&key->lock);
  key->ref_count++;
  spinlock_unlock(&key->lock);

  dtls_list_element_t *e = NULL;
  TAILQ_FOREACH(e, current_dtls_list, link)
    if(e->key == key) break;
  if(!e) {
    e = __libc_malloc(sizeof(dtls_list_element_t));
    assert(e);
    e->key = key;
    TAILQ_INSERT_HEAD(current_dtls_list, e, link);
  }
  e->dtls = dtls;
}

void *get_dtls(dtls_key_t *key)
{
  assert(key);
  if(current_dtls_list == NULL)
    return NULL;

  dtls_list_element_t *e = NULL;
  TAILQ_FOREACH(e, current_dtls_list, link)
    if(e->key == key) return e->dtls;
  return e;
}

void destroy_dtls() {
  if(current_dtls_list == NULL)
    return;

  dtls_list_element_t *e,*n;
  e = TAILQ_FIRST(current_dtls_list);
  while(e != NULL) {
    dtls_key_t *key = e->key;
    bool run_dtor = false;
  
    spinlock_lock(&key->lock);
    if(key->valid)
      if(key->dtor)
        run_dtor = true;
    spinlock_unlock(&key->lock);

	// MUST run the dtor outside the spinlock if we want it to be able to call
	// code that may deschedule it for a while (i.e. a mutex). Probably a
	// good idea anyway since it can be arbitrarily long and is written by the
	// user. Note, there is a small race here on the valid field, whereby we
	// may run a destructor on an invalid key. At least the keys memory wont
	// be deleted though, as protected by the ref count. Any reasonable usage
	// of this interface should safeguard that a key is never destroyed before
	// all of the threads that use it have exited anyway.
    if(run_dtor) {
	  void *dtls = e->dtls;
      e->dtls = NULL;
      key->dtor(dtls);
    }

    spinlock_lock(&key->lock);
    key->ref_count--;
    spinlock_unlock(&key->lock);
    if(key->ref_count == 0)
      __libc_free(key);

    n = TAILQ_NEXT(e, link);
    TAILQ_REMOVE(current_dtls_list, e, link);
    __libc_free(e);
    e = n;
  }
}

