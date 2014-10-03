/*
 * Copyright (c) 2014 The Regents of the University of California
 * Andrew Waterman <waterman@cs.berkeley.edu>
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "atomic.h"
#include "tls.h"
#include "vcore.h"
#include "waitfreelist.h"

#define printf_safe(...)           \
  printf(__VA_ARGS__)

#define NUM_VCORES \
  max_vcores()

volatile int b1, b2;

struct wfl wfl = WFL_INITIALIZER(wfl);

void vcore_entry()
{
  if(vcore_saved_ucontext) {
    void *cuc = vcore_saved_ucontext;
    printf_safe("Restoring context: entry %d, num_vcores: %ld\n", vcore_id(), num_vcores());
    set_tls_desc(current_tls_desc, vcore_id());
    parlib_setcontext(cuc);
    assert(0);
  }

  __sync_fetch_and_add(&b1, 1);
  while (b1 < NUM_VCORES);

  if (vcore_id() == 0) {
    while (b2 < NUM_VCORES-1)
      printf("capacity %ld size %ld\n", wfl_capacity(&wfl), wfl_size(&wfl));
  } else if (vcore_id() > 1) {
    for (int i = 0; i < 1000000; i++) {
      if (vcore_id() % 2 == 0)
        wfl_insert(&wfl, (void*)(uintptr_t)vcore_id());
      else
        wfl_remove(&wfl);
    }
  }


  __sync_fetch_and_add(&b2, 1);
  while (b2 < NUM_VCORES);

  if (vcore_id() == 0) {
    wfl_cleanup(&wfl);
    exit(0);
  }

  vcore_yield();
}

int main()
{
  vcore_lib_init();
  printf_safe("main, max_vcores: %ld\n", max_vcores());
  vcore_request(NUM_VCORES);
  set_tls_desc(vcore_tls_descs[0], 0);
  vcore_saved_ucontext = NULL;  
  vcore_entry();
}

