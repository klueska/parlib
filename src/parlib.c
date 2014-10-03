/*
 * Copyright (c) 2013 The Regents of the University of California
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

#include "internal/parlib.h"
#include "parlib.h"
#include <pthread.h>

void parlib_get_main_stack(void **bottom, size_t *size)
{
  /* Fill in the main context stack info. Get the top of the stack by
   * examining &attr.  Since that's not quite true, divide the stack size by
   * two to conservatively compute the stack bottom. Then page align it. */
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_getstacksize(&attr, size);
  *bottom = (void*)(((uintptr_t)&attr - *size/2) & -getpagesize());
}
