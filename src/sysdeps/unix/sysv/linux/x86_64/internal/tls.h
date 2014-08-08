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

#ifndef PARLIB_TLS_INTERNAL_H
#define PARLIB_TLS_INTERNAL_H

#include "config.h"
#include "assert.h"
#include <unistd.h>
#include <sys/syscall.h>

#include "arch.h"

#include <asm/prctl.h>
#include <sys/prctl.h>
#include "fsgs.h"

int arch_prctl(int code, unsigned long *addr);

/* Get the current tls base address */
static __inline void *get_current_tls_base()
{
  uintptr_t addr;
#ifdef HAVE_FSGSBASE
  addr = rdfsbase();
#else
  int ret = arch_prctl(ARCH_GET_FS, &addr);
  assert(ret == 0);
#endif
  return (void *)addr;
}

typedef void *arch_tls_data_t;
#define TLS_DESC(tls_data) (tls_data)
#define CLONE_TLS_PARAM(tls_data) (tls_data)

/* Set the current tls base address */
static __inline void set_current_tls_base(void *tls_desc,
                                          arch_tls_data_t *data)
{
  *data = tls_desc;
#ifdef HAVE_FSGSBASE
  wrfsbase((uintptr_t)tls_desc);
#else
  int ret = arch_prctl(ARCH_SET_FS, tls_desc);
  assert(ret == 0);
#endif
}

static void init_arch_tls_data(arch_tls_data_t *data, void *tls_desc,
                               uint32_t vcoreid)
{
  *data = tls_desc;
}

#endif /* PARLIB_TLS_INTERNAL_H */
