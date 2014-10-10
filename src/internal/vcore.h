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

#ifndef VCORE_INTERNAL_H
#define VCORE_INTERNAL_H

/* #include this file for all internal modules so they get access to the type
 * definitions and externed variables below */

#include <stdbool.h>
#include <asm/ldt.h>
#include <bits/local_lim.h>

#include "arch.h"
#include "parlib-config.h"
#include "internal/tls.h"
#include "atomic.h"

#include <pthread.h>
#include "limits.h"
#define VCORE_STACK_SIZE (4*PTHREAD_STACK_MIN)

struct vcore {
  /* For bookkeeping */
  atomic_t allocated;

  /* Architecture-specific TLS context information, e.g. LDT on IA-32 or
     the address of the TCB on other ISAs. */
  arch_tls_data_t arch_tls_data;
};
/* Array of vcores */
extern struct vcore *__vcores;

void *__stack_alloc(size_t s);
void __stack_free(void *stack, size_t s);

pthread_t internal_pthread_create(pthread_attr_t *attr,
                                  void *(*start_routine) (void *), void *arg);

/* Architecture-specific function to reenter atop the stack. */
extern void __attribute__((noinline,noreturn))
  __vcore_reenter(void (*func)(void), void *sp);

#endif // VCORE_INTERNAL_H
