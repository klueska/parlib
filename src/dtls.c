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
#include "spinlock.h"
#include "slab.h"
#include "internal/assert.h"

/* The current dymamic tls implementation uses a locked linked list
 * to find the key for a given thread. We should probably find a better way to
 * do this based on a custom lock-free hash table or something. */
#include <sys/queue.h>
#include "spinlock.h"

/* The dynamic tls key structure */
struct dtls_key {
  int ref_count;
  bool valid;
  void (*dtor)(void*);
};

/* The definition of a dtls_key list and its elements */
struct dtls_value {
  TAILQ_ENTRY(dtls_value) link;
  struct dtls_key *key;
  void *dtls;
}; 
TAILQ_HEAD(dtls_list, dtls_value);

/* A struct containing all of the per thread (i.e. vcore or uthread) data
 * associated with dtls */
typedef struct dtls_data {
  /* A per-thread list of dtls regions */
  struct dtls_list list;

} dtls_data_t;

/* A slab of dtls keys (global to all threads) */
static struct slab_cache *__dtls_keys_cache;

/* A slab of values for use when mapping a dtls_key to its per-thread value */
struct slab_cache *__dtls_values_cache;
  
/* A slab of dtls data for per-thread management */
struct slab_cache *__dtls_data_cache;
  
/* A lock protecting access to the caches above */
static spinlock_t __slab_lock;

static __thread dtls_data_t __dtls_data;
static __thread bool __dtls_initialized = false;

#ifdef PARLIB_NO_UTHREAD_TLS
#include "uthread.h"
#endif

static dtls_key_t __allocate_dtls_key() 
{
  spinlock_lock(&__slab_lock);
  dtls_key_t key = slab_cache_alloc(__dtls_keys_cache, 0);
  spinlock_unlock(&__slab_lock);
  assert(key);
  key->ref_count = 1;
  return key;
}

static void __maybe_free_dtls_key(dtls_key_t key)
{
  if(key->ref_count == 0) {
    spinlock_lock(&__slab_lock);
    slab_cache_free(__dtls_keys_cache, key);
    spinlock_unlock(&__slab_lock);
  }
}

/* Constructor to get a reference to the main thread's TLS descriptor */
static void dtls_lib_init()
{
	/* Make sure this only runs once */
  run_once(
      /* Initialize the global cache of dtls_keys */
	  __dtls_keys_cache = slab_cache_create("dtls_keys_cache", 
        sizeof(struct dtls_key), __alignof__(struct dtls_key), 0, NULL, NULL);

	  __dtls_values_cache = slab_cache_create("dtls_values_cache", 
        sizeof(struct dtls_value), __alignof__(struct dtls_value), 0, NULL, NULL);

	  __dtls_data_cache = slab_cache_create("dtls_data_cache", 
        sizeof(struct dtls_data), __alignof__(struct dtls_data), 0, NULL, NULL);

    /* Initialize the lock that protects the cache */
    spinlock_init(&__slab_lock);
  );
}

dtls_key_t dtls_key_create(dtls_dtor_t dtor)
{
  dtls_lib_init();
  dtls_key_t key = __allocate_dtls_key();
  key->valid = true;
  key->dtor = dtor;
  return key;
}

void dtls_key_delete(dtls_key_t key)
{
  assert(key);
  __sync_fetch_and_add(&key->ref_count, -1);
  key->valid = false;
  __maybe_free_dtls_key(key);
}

static inline void __set_dtls(dtls_data_t *dtls_data, dtls_key_t key, void *dtls)
{
  assert(key);
  __sync_fetch_and_add(&key->ref_count, 1);

  struct dtls_value *v = NULL;
  TAILQ_FOREACH(v, &dtls_data->list, link)
    if(v->key == key) break;

  if(!v) {
    spinlock_lock(&__slab_lock);
    v = slab_cache_alloc(__dtls_values_cache, 0);
    spinlock_unlock(&__slab_lock);
    assert(v);
    v->key = key;
    TAILQ_INSERT_HEAD(&dtls_data->list, v, link);
  }
  v->dtls = dtls;
}

static inline void *__get_dtls(dtls_data_t *dtls_data, dtls_key_t key)
{
  assert(key);

  struct dtls_value *v = NULL;
  TAILQ_FOREACH(v, &dtls_data->list, link)
    if(v->key == key) return v->dtls;
  return v;
}

static inline void __destroy_dtls(dtls_data_t *dtls_data)
{
 struct dtls_value *v,*n;
  v = TAILQ_FIRST(&dtls_data->list);
  while(v != NULL) {
    dtls_key_t key = v->key;
  
    /* Note, there is a small race here on the valid field, whereby we may run
     * a destructor on an invalid key. At least the keys memory wont be deleted
     * though, as protected by the ref count. Any reasonable usage of this
     * interface should safeguard that a key is never destroyed before all of the
     * threads that use it have exited anyway. */
    if(key->dtor) {
      if(key->valid) {
        void *dtls = v->dtls;
        v->dtls = NULL;
        key->dtor(dtls);
      }
    }
    __sync_fetch_and_add(&key->ref_count, -1);
    __maybe_free_dtls_key(key);

    n = TAILQ_NEXT(v, link);
    TAILQ_REMOVE(&dtls_data->list, v, link);
    spinlock_lock(&__slab_lock);
    slab_cache_free(__dtls_values_cache, v);
    spinlock_unlock(&__slab_lock);
    v = n;
  }
}

void set_dtls(dtls_key_t key, void *dtls)
{
  bool initialized = true;
  dtls_data_t *dtls_data = NULL;
#ifdef PARLIB_NO_UTHREAD_TLS
  if(!in_vcore_context()) {
    assert(current_uthread);
    if(current_uthread->dtls_data == NULL) {
      spinlock_lock(&__slab_lock);
      current_uthread->dtls_data = slab_cache_alloc(__dtls_data_cache, 0);
      spinlock_unlock(&__slab_lock);
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
    TAILQ_INIT(&dtls_data->list);
  }
  __set_dtls(dtls_data, key, dtls);
}

void *get_dtls(dtls_key_t key)
{
  dtls_data_t *dtls_data = NULL;
#ifdef PARLIB_NO_UTHREAD_TLS
  if(!in_vcore_context()) {
    assert(current_uthread);
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
  spinlock_lock(&__slab_lock);
  slab_cache_free(__dtls_data_cache, dtls_data);
  spinlock_unlock(&__slab_lock);
#endif
}

