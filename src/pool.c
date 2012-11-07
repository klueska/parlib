/*
 * Copyright (c) 2006 Stanford University.
 * Copyright (c) 2012 The Regents of the University of California
 * All rights reserved.
 */

/**
 *  An allocation pool of a specific type memory objects.  The Pool allows
 *  components to allocate and free elements. The pool does not require that
 *  deallocations be items which were originally allocated. E.g., a program can
 *  create two pools of the same type and pass items between them.
 *
 *  @author Philip Levis
 *  @author Kyle Jamieson
 *  @author Geoffrey Mainland
 *  @author Kevin Klues <klueska@cs.berkeley.edu>
 *  @date   $Date: 2010-01-20 19:59:07 $
 */

#include <stddef.h>
#include <assert.h>
#include "pool.h"

void pool_init(pool_t *pool, void* buffer, void **object_queue,
               size_t num_objects, size_t object_size)
{
  assert(pool);
  assert(buffer);
  assert(object_queue);
  assert(num_objects > 0);
  assert(object_size > 0);

  for (int i = 0; i < num_objects; i++) {
    object_queue[i] = buffer + object_size*i;
  }
  pool->buffer = buffer;
  pool->object_queue = object_queue;
  pool->num_objects = num_objects;
  pool->object_size = object_size;
  pool->free = num_objects;
  pool->index = 0;
}

size_t pool_size(pool_t *pool)
{
  return pool->num_objects;
}

size_t pool_available(pool_t *pool)
{
  assert(pool);
  return pool->free;
}

void* pool_alloc(pool_t *pool)
{
  assert(pool);

  if (pool->free) {
    void* rval = pool->object_queue[pool->index];
    pool->object_queue[pool->index] = NULL;
    pool->free--;
    pool->index++;
    if (pool->index == pool->num_objects) {
      pool->index = 0;
    }
    return rval;
  }
  return NULL;
}

int pool_free(pool_t* pool, void *object)
{
  assert(pool);

  if (pool->free >= pool->num_objects) {
    return -1;
  }
  else {
    uint16_t emptyIndex = (pool->index + pool->free);
    if (emptyIndex >= pool->num_objects) {
      emptyIndex -= pool->num_objects;
    }
    pool->object_queue[emptyIndex] = object;
    pool->free++;
    return 0;
  }
}

