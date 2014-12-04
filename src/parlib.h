/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
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

// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef PARLIB_PARLIB_H
#define PARLIB_PARLIB_H 1

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#include "parlib-config.h"
#include "atomic.h"
#include "arch.h"
#include "vcore.h"
#include "uthread.h"
#include "mcs.h"
#include "tls.h"
#include "dtls.h"
#include "spinlock.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#ifndef NULL
#define NULL ((void*) 0)
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

enum {
	PG_RDONLY = 4,
	PG_RDWR   = 6,
};

/* Function to obtain the main thread's stack bottom and size. Must be called
 * while running on the main thread, otherwise the values returned are
 * undefined. */
void EXPORT_SYMBOL parlib_get_main_stack(void **bottom, size_t *size);

#define CACHE_LINE_ALIGNED __attribute__((aligned(ARCH_CL_SIZE)))

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

#define FOR_CIRC_BUFFER(next, size, var) \
	for (int _var = 0, var = (next); _var < (size); _var++, var = (var + 1) % (size))

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)						\
({								\
	uintptr_t __a = (uintptr_t) (a);				\
	(typeof(a)) (__a - __a % (n));				\
})

// Round up to the nearest multiple of n
#define ROUNDUP(a, n)						\
({								\
	uintptr_t __n = (uintptr_t) (n);				\
	(typeof(a)) (ROUNDDOWN((uintptr_t) (a) + __n - 1, __n));	\
})

// Round down to the nearest multiple of n
#define PTRROUNDDOWN(a, n)						\
({								\
	char * __a = (char *) (a);				\
	(typeof(a)) (__a - (uintptr_t)__a % (n));				\
})
// Round pointer up to the nearest multiple of n
#define PTRROUNDUP(a, n)						\
({								\
	uintptr_t __n = (uintptr_t) (n);				\
	(typeof(a)) (PTRROUNDDOWN((char *) (a) + __n - 1, __n));	\
})

#define __b2(x)   (     (x) | (     (x) >> 1) )
#define __b4(x)   ( __b2(x) | ( __b2(x) >> 2) )
#define __b8(x)   ( __b4(x) | ( __b4(x) >> 4) )
#define __b16(x)  ( __b8(x) | ( __b8(x) >> 8) )
#define __b32(x)  (__b16(x) | (__b16(x) >>16) )
#define __b64(x)  (__b32(x) | (__b32(x) >>32) )
#define NEXTPOWER2(x) (__b64((uint64_t)(x)-1) + 1)

// Return the integer logarithm of the value provided rounded down
static inline uintptr_t LOG2_DOWN(uintptr_t value)
{
	uintptr_t l = 0;
	while( (value >> l) > 1 ) ++l;
	return l;
}

// Return the integer logarithm of the value provided rounded up
static inline uintptr_t LOG2_UP(uintptr_t value)
{
	uintptr_t _v = LOG2_DOWN(value);
	if (value ^ (1 << _v))
		return _v + 1;
	else
		return _v;
}

static inline uintptr_t ROUNDUPPWR2(uintptr_t value)
{
	return 1 << LOG2_UP(value);
}

#define parlib_malloc(size) \
({ \
  void *p = malloc(size); \
  if (p == NULL) abort(); \
  p; \
})

#define parlib_aligned_alloc(align, size) \
({ \
  void *p; \
  if (posix_memalign(&p, NEXTPOWER2(align), (size))) abort(); \
  p; \
})

/* Makes sure func is run exactly once.  Can handle concurrent callers, and
 * other callers spin til the func is complete. Do NOT execute a return
 * statement within func. */
#define run_once(func)                                                   \
{                                                                        \
  static bool ran_once = FALSE;                                          \
  static atomic_t is_running = FALSE;                                    \
  if (!ran_once) {                                                       \
    if (!atomic_swap(&is_running, TRUE)) {                               \
      /* we won the race and get to run the func */                      \
      func;                                                              \
      wmb();  /* don't let the ran_once write pass previous writes */    \
      ran_once = TRUE;                                                   \
    } else {                                                             \
      /* someone else won, wait til they are done to break out */        \
      while (!ran_once)                                                  \
        cpu_relax();                                                     \
                                                                         \
    }                                                                    \
  }                                                                      \
}

/* Unprotected, single-threaded version, makes sure func is run exactly once.
 * Do NOT execute a return statement within func. */
#define run_once_racy(func)                                                  \
{                                                                            \
  static bool ran_once = FALSE;                                              \
  if (!ran_once) {                                                           \
    func;                                                                    \
    ran_once = TRUE;                                                         \
  }                                                                          \
}

/* Aborts with 'retcmd' if this function has already been called.  Compared to
 * run_once, this is put at the top of a function that can be called from
 * multiple sources but should only execute once. */
#define init_once_racy(retcmd)                                             \
{                                                                          \
  static bool initialized = FALSE;                                         \
  if (initialized) {                                                       \
    retcmd;                                                                \
  }                                                                        \
  initialized = TRUE;                                                      \
}

#endif  // PARLIB_PARLIB_H
