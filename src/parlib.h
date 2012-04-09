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

#ifndef INC_PARLIB_H
#define INC_PARLIB_H 1

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <assert.h>

#include "parlib-config.h"
#include "atomic.h"
#include "arch.h"
#include "vcore.h"
#include "syscall.h"
#include "uthread.h"
#include "mcs.h"
#include "tls.h"
#include "spinlock.h"

enum {
	PG_RDONLY = 4,
	PG_RDWR   = 6,
};

enum {
	FALSE = 0,
	TRUE
};

#endif	// !ASSEMBLER

#endif	// !INC_PARLIB_H
