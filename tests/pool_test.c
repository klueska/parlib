/*
 * Copyright (c) 2012 The Regents of the University of California
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

#include "internal/assert.h"
#include <stdio.h>

#include "pool.h"

#define NUM_OBJECTS 1000

int main(int argc, char** argv)
{
  pool_t pool;
  int buffer[NUM_OBJECTS];
  void *queue[NUM_OBJECTS];
  pool_init(&pool, buffer, queue, NUM_OBJECTS, sizeof(int));
  printf("pool_size: %lu, pool_available: %lu\n", pool_size(&pool), pool_available(&pool));

  int *test_buffer[NUM_OBJECTS];
  for(int i=0; i<NUM_OBJECTS; i++) {
    test_buffer[i] = (int*)pool_alloc(&pool);
    *test_buffer[i] = i;
    printf("pool_available: %lu\n", pool_available(&pool));
  }

  if(pool_available(&pool) == 0)
    printf("Pool empty! As it should be...\n");
  else
    printf("Pool not empty! Something is afoot...\n");

  for(int i=0; i<NUM_OBJECTS; i++) {
    printf("test_buffer[%d] = %d, pool_available: %lu\n", i, *test_buffer[i], pool_available(&pool));
    pool_free(&pool, test_buffer[i]);
  }
  printf("pool_available: %lu\n", pool_available(&pool));
}

