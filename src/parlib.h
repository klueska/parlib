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
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <assert.h>

#include "parlib-config.h"
#include "atomic.h"
#include "arch.h"
#include "vcore.h"
#include "uthread.h"
#include "mcs.h"
#include "tls.h"
#include "dtls.h"
#include "spinlock.h"

#ifndef NULL
#define NULL ((void*) 0)
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif


/* Constructor to set up the parlib library */
int parlib_init(struct uthread *main_thread);

enum {
	PG_RDONLY = 4,
	PG_RDWR   = 6,
};

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

#endif	// PARLIB_PARLIB_H
