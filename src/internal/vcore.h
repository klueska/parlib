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

#ifdef PARLIB_VCORE_AS_PTHREAD
  #include <pthread.h>
#endif

#define VCORE_MIN_STACK_SIZE (3*PGSIZE)

struct vcore {
  /* For bookkeeping */
  bool created __attribute__((aligned (ARCH_CL_SIZE)));
  bool allocated __attribute__((aligned (ARCH_CL_SIZE)));
  bool running __attribute__((aligned (ARCH_CL_SIZE)));

#ifdef __i386___
  /* The ldt entry associated with this vcore. Used for managing TLS in user
   * space. */
  struct user_desc ldt_entry;
#elif __x86_64__
  /* The base address of the TLS currently installed on this vcore. Used for
   * managing TLS in user space. */
  void *current_tls_base;
#endif
  
#ifdef PARLIB_VCORE_AS_PTHREAD
  /* The linux pthread associated with this vcore */
  pthread_t thread;
#else
  /* Thread properties when running in vcore context: stack + TLS stuff */
  pid_t ptid;
  void *stack_bottom;
  size_t stack_size;
  void *tls_desc;
#endif
};

#endif // VCORE_INTERNAL_H
