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

#ifndef PARLIB_PARLIB_H
#define PARLIB_PARLIB_H

#include "common.h"
#include "parlib-config.h"
#include "atomic.h"
#include "arch.h"
#include "context.h"
#include "vcore.h"
#include "uthread.h"
#include "mcs.h"
#include "tls.h"
#include "dtls.h"
#include "spinlock.h"
#include "export.h"

/* Function to obtain the main thread's stack bottom and size. Must be called
 * while running on the main thread, otherwise the values returned are
 * undefined. */
void EXPORT_SYMBOL parlib_get_main_stack(void **bottom, size_t *size);

#endif  // PARLIB_PARLIB_H
