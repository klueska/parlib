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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "atomic.h"
#include "tls.h"
#include "vcore.h"

#ifndef PARLIB_VCORE_AS_PTHREAD
#include "spinlock.h"
spinlock_t printf_lock = SPINLOCK_UNLOCKED;
#define printf_safe(...)           \
{                                  \
  spinlock_lock(&printf_lock);     \
  printf(__VA_ARGS__);             \
  spinlock_unlock(&printf_lock);   \
}
#else
#define printf_safe(...)           \
  printf(__VA_ARGS__)
#endif

#define NUM_VCORES \
  max_vcores()

void vcore_entry()
{
  if(vcore_saved_ucontext) {
    void *cuc = vcore_saved_ucontext;
    printf_safe("Restoring context: entry %d, num_vcores: %ld\n", vcore_id(), num_vcores());
    set_tls_desc(current_tls_desc, vcore_id());
    setcontext(cuc);
    assert(0);
  }

  printf_safe("entry %d, num_vcores: %ld\n", vcore_id(), num_vcores());
  vcore_request(1);
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

