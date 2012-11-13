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
#include "dtls.h"
#include "mcs.h"
#include "pool.h"

/* The current dymamic tls implementation uses a locked linked list
 * to find the key for a given thread. We should probably find a better way to
 * do this based on a custom lock-free hash table or something. */
#include "queue.h"
#include "spinlock.h"

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
  dtls_key_t key;
  void *dtls;
} dtls_list_element_t;
TAILQ_HEAD(dtls_list, dtls_list_element);
typedef struct dtls_list dtls_list_t;

/* A statically allocated buffer of dtls keys (global to all threads) */
static struct dtls_key __dtls_keys[DTLS_KEYS_MAX];

/* A statically allocated object queue and pool to manage the buffer of dtls keys */
static void *__dtls_keys_queue[DTLS_KEYS_MAX];
static pool_t __dtls_keys_pool;

/* A lock protecting access to the global pool of dtls keys */
static mcs_lock_t __dtls_keys_pool_lock;

/* A per-thread buffer of dtls_list_elements for use when mapping a dtls_key to
 * its per-thread value */
static __thread struct dtls_list_element __dtls_list_elements[DTLS_KEYS_MAX];

/* A per-thread object queue and pool to manage the per-thread buffer of
 * dtls_list_elements */
static __thread void *__dtls_list_elements_queue[DTLS_KEYS_MAX];
static __thread pool_t __dtls_list_elements_pool;

/* A per-thread list of dynamically allocatable tls regions */
static __thread dtls_list_t __dtls_list;

/* A per-thread pointer to the current list of dynamically allocatable tls regions */
static __thread dtls_list_t *current_dtls_list;

/* Constructor to get a reference to the main thread's TLS descriptor */
int dtls_lib_init()
{
	/* Make sure this only runs once */
	static bool initialized = false;
	if (initialized)
	    return 0;
	initialized = true;
	
    /* Initialize the global pool of dtls_keys */
    pool_init(&__dtls_keys_pool, __dtls_keys, __dtls_keys_queue,
              DTLS_KEYS_MAX, sizeof(struct dtls_key));
    /* Initialize the lock that protects the pool */
    mcs_lock_init(&__dtls_keys_pool_lock);
	return 0;
}

dtls_key_t dtls_key_create(dtls_dtor_t dtor)
{
  mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
  mcs_lock_lock(&__dtls_keys_pool_lock, &qnode);
  dtls_key_t key = pool_alloc(&__dtls_keys_pool);
  mcs_lock_unlock(&__dtls_keys_pool_lock, &qnode);
  assert(key);

  spinlock_init(&key->lock);
  key->ref_count = 1;
  key->valid = true;
  key->dtor = dtor;
  return key;
}

void dtls_key_delete(dtls_key_t key)
{
  assert(key);

  spinlock_lock(&key->lock);
  key->valid = false;
  key->ref_count--;
  spinlock_unlock(&key->lock);
  if(key->ref_count == 0) {
    mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
    mcs_lock_lock(&__dtls_keys_pool_lock, &qnode);
    pool_free(&__dtls_keys_pool, key);
    mcs_lock_unlock(&__dtls_keys_pool_lock, &qnode);
  }
}

void set_dtls(dtls_key_t key, void *dtls)
{
  assert(key);
  if(current_dtls_list == NULL) {
    pool_init(&__dtls_list_elements_pool, __dtls_list_elements, 
              __dtls_list_elements_queue, DTLS_KEYS_MAX,
              sizeof(struct dtls_list_element));
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
    e = pool_alloc(&__dtls_list_elements_pool);
    assert(e);
    e->key = key;
    TAILQ_INSERT_HEAD(current_dtls_list, e, link);
  }
  e->dtls = dtls;
}

void *get_dtls(dtls_key_t key)
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
    dtls_key_t key = e->key;
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
      pool_free(&__dtls_keys_pool, key);

    n = TAILQ_NEXT(e, link);
    TAILQ_REMOVE(current_dtls_list, e, link);
    pool_free(&__dtls_list_elements_pool, e);
    e = n;
  }
}

