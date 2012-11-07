/*
 * Copyright (c) 2012 The Regents of the University of California
 * All rights reserved.
 * Kevin Klues <klueska@cs.berkeley.edu>
 */

#include <stdint.h>
#include <stdbool.h>

/* Metadata needed to manage a pool */
typedef struct pool {
  void *buffer;
  void **object_queue;
  size_t num_objects;
  size_t object_size;
  unsigned int free;
  unsigned int index;
} pool_t;

/* Initialize a pool.  All memory MUST be allocated externally.  The pool
 * implementation simply manages the objects contained in the buffer passed to
 * this function.  This allows us to use the same pool implementation for both
 * statically and dynamically allocated memory. */
void pool_init(pool_t *pool, void* buffer, void **object_queue,
               size_t num_objects, size_t object_size);

/* Check how many objects the pool is able to hold */ 
size_t pool_size(pool_t *pool);

/* See how many objects are currently available for allocation from the pool. */
size_t pool_available(pool_t *pool);

/* Get an object from the pool */
void* pool_alloc(pool_t *pool);

/* Put an object into the pool */
int pool_free(pool_t* pool, void *object);

