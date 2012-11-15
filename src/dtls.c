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

/* A struct containing all of the per thread (i.e. vcore or uthread) data
 * associated with dtls */
typedef struct dtls_data {
  /* A per-thread list of dtls regions */
  dtls_list_t list;

  /* A per-thread buffer of dtls_list_elements for use when mapping a dtls_key to
   * its per-thread value */
  struct dtls_list_element list_elements[DTLS_KEYS_MAX];
  
  /* A per-thread object queue and pool to manage the per-thread buffer of
   * dtls_list_elements */
  void *list_elements_queue[DTLS_KEYS_MAX];
  pool_t list_elements_pool;
  
} dtls_data_t;
static __thread dtls_data_t __dtls_data;
static __thread bool __dtls_initialized = false;

#ifdef PARLIB_NO_UTHREAD_TLS
#include "uthread.h"
#endif

static dtls_key_t __alocate_dtls_key() 
{
  mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
  mcs_lock_lock(&__dtls_keys_pool_lock, &qnode);
  dtls_key_t key = pool_alloc(&__dtls_keys_pool);
  assert(key);
  key->ref_count = 1;
  mcs_lock_unlock(&__dtls_keys_pool_lock, &qnode);
  return key;
}

static void __maybe_free_dtls_key(dtls_key_t key)
{
  if(key->ref_count == 0) {
    mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
    mcs_lock_lock(&__dtls_keys_pool_lock, &qnode);
    pool_free(&__dtls_keys_pool, key);
    mcs_lock_unlock(&__dtls_keys_pool_lock, &qnode);
  }
}

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
  dtls_key_t key = __alocate_dtls_key();
  spinlock_init(&key->lock);
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
  __maybe_free_dtls_key(key);
}

void inline __set_dtls(dtls_data_t *dtls_data, dtls_key_t key, void *dtls)
{
  assert(key);

  spinlock_lock(&key->lock);
  key->ref_count++;
  spinlock_unlock(&key->lock);

  dtls_list_element_t *e = NULL;
  TAILQ_FOREACH(e, &dtls_data->list, link)
    if(e->key == key) break;

  if(!e) {
    e = pool_alloc(&dtls_data->list_elements_pool);
    assert(e);
    e->key = key;
    TAILQ_INSERT_HEAD(&dtls_data->list, e, link);
  }
  e->dtls = dtls;
}

static inline void *__get_dtls(dtls_data_t *dtls_data, dtls_key_t key)
{
  assert(key);

  dtls_list_element_t *e = NULL;
  TAILQ_FOREACH(e, &dtls_data->list, link)
    if(e->key == key) return e->dtls;
  return e;
}

static inline void __destroy_dtls(dtls_data_t *dtls_data)
{
  dtls_list_element_t *e,*n;
  e = TAILQ_FIRST(&dtls_data->list);
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
    __maybe_free_dtls_key(key);

    n = TAILQ_NEXT(e, link);
    TAILQ_REMOVE(&dtls_data->list, e, link);
    pool_free(&dtls_data->list_elements_pool, e);
    e = n;
  }
}

void set_dtls(dtls_key_t key, void *dtls)
{
  bool initialized = true;
  dtls_data_t *dtls_data = NULL;
#ifdef PARLIB_NO_UTHREAD_TLS
  if(!in_vcore_context()) {
    if(current_uthread->dtls_data == NULL) {
      current_uthread->dtls_data = malloc(sizeof(dtls_data_t));
      initialized = false;
    }
    dtls_data = current_uthread->dtls_data;
  }
  else {
#endif
    if(!__dtls_initialized) {
      initialized = false;
      __dtls_initialized  = true;
    }
    dtls_data = &__dtls_data;
#ifdef PARLIB_NO_UTHREAD_TLS
  }
#endif
  if(!initialized) {
    pool_init(&dtls_data->list_elements_pool,
              dtls_data->list_elements, 
              dtls_data->list_elements_queue,
              DTLS_KEYS_MAX,
              sizeof(struct dtls_list_element));
    TAILQ_INIT(&dtls_data->list);
  }
  __set_dtls(dtls_data, key, dtls);
}

void *get_dtls(dtls_key_t key)
{
  dtls_data_t *dtls_data = NULL;
#ifdef PARLIB_NO_UTHREAD_TLS
  if(!in_vcore_context()) {
    if(current_uthread->dtls_data == NULL)
      return NULL;
    dtls_data = current_uthread->dtls_data;
  }
  else {
#endif
    if(!__dtls_initialized)
      return NULL;
    dtls_data = &__dtls_data;
#ifdef PARLIB_NO_UTHREAD_TLS
  }
#endif
  return __get_dtls(dtls_data, key);
}

void destroy_dtls()
{
  dtls_data_t *dtls_data = NULL;
#ifdef PARLIB_NO_UTHREAD_TLS
  if(!in_vcore_context()) {
    if(current_uthread->dtls_data == NULL)
      return;
    dtls_data = current_uthread->dtls_data;
  }
  else {
#endif
    if(!__dtls_initialized)
      return;
    dtls_data = &__dtls_data;
#ifdef PARLIB_NO_UTHREAD_TLS
  }
#endif

  __destroy_dtls(dtls_data);

#ifdef PARLIB_NO_UTHREAD_TLS
  free(dtls_data);
#endif
}

