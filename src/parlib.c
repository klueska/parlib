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

#include "internal/tls.h"
#include "internal/dtls.h"
#include "internal/vcore.h"
#include "internal/uthread.h"

/* Constructor to set up the parlib library */
int parlib_init(struct uthread *main_thread)
{
  assert(!dtls_lib_init());
  assert(!tls_lib_init());
  assert(!vcore_lib_init());
  assert(!uthread_lib_init(main_thread));
  return 0;
}

