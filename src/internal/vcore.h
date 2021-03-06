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
#include <sys/queue.h>
#include <signal.h>

#include "arch.h"
#include "parlib-config.h"
#include "atomic.h"

#include <pthread.h>
#include "limits.h"
#define VCORE_STACK_SIZE (4*PTHREAD_STACK_MIN)

#define SIGVCORE	SIGUSR1

struct vcore_sigstack {
	SLIST_ENTRY(vcore_sigstack) next;
	void *sigstack[SIGSTKSZ - sizeof(SLIST_ENTRY(vcore_sigstack))];
};
SLIST_HEAD(vcore_sigstack_list, vcore_sigstack);

struct vcore {
  /* For bookkeeping */
  atomic_t allocated;
  atomic_t requested;

#ifdef arch_tls_data_t
  /* Architecture-specific TLS context information, e.g. LDT on IA-32 */
  arch_tls_data_t arch_tls_data;
#endif

  /* List of sigstacks for use on this vcore. Grows, but never shrinks. */
  struct vcore_sigstack_list sigstacklist;
  void *activesigstack;

  /* Pointer to the backing pthread for this vcore */
  pthread_t pthread;
};

/* Internal cache aligned, per vcore data */
struct internal_vcore_pvc_data {
	/* The vcore struct itself */
	struct vcore vcore;

	/* Marker indicating that the vcore is not able to handle a signal
	 * immediately */
	atomic_t sigpending;
} __attribute((aligned(ARCH_CL_SIZE)));
extern struct internal_vcore_pvc_data *internal_vcore_pvc_data;
#define __vcores(i) (internal_vcore_pvc_data[i].vcore)
#define __vcore_sigpending(i) (internal_vcore_pvc_data[i].sigpending)

void __sigstack_swap(void **sigstack);
void __sigstack_free(void **sigstack);

pthread_t internal_pthread_create(size_t stack_size,
                                  void *(*start_routine) (void *), void *arg);

/* Architecture-specific function to reenter atop the stack. */
extern void __attribute__((noinline,noreturn))
  __vcore_reenter(void (*func)(void), void *sp);

#endif // VCORE_INTERNAL_H
