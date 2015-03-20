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

#ifndef PARLIB_ARCH_H
#define PARLIB_ARCH_H 

#ifdef __linux__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
#include "parlib-config.h"

#define internal_function

#define PGSIZE getpagesize()
#define ARCH_CL_SIZE 128 // 2*64 to account for Adjacent CL Prefetcher

static __inline void
breakpoint(void)
{
	__asm __volatile("int3");
}

static __inline void
cpu_relax(void)
{
	asm volatile("pause" : : : "memory");
}

static __inline uint64_t                                                                             
read_pmc(uint32_t index)
{                                                                                                    
    uint64_t pmc;

    __asm __volatile("rdpmc" : "=A" (pmc) : "c" (index)); 
    return pmc;                                                                                      
}

int arch_prctl(int code, unsigned long *addr);

/* Get the current tls base address */
static __inline void *get_current_tls_base()
{
  uintptr_t addr;
#ifdef PARLIB_HAVE_FSGSBASE
	asm volatile("rdfsbase %%rax"
			: "=a" (addr)
			:: "memory");
#else
  arch_prctl(ARCH_GET_FS, &addr);
#endif
  return (void *)addr;
}

/* Set the current tls base address */
static __inline void set_current_tls_base(void *tls_desc)
{
#ifdef PARLIB_HAVE_FSGSBASE
	asm volatile("wrfsbase %%rax"
			:: "a" (tls_desc)
			: "memory");
#else
  arch_prctl(ARCH_SET_FS, (uintptr_t *)tls_desc);
#endif
}

#else // !__linux__
  #error "Your architecture is not yet supported by parlib!"
#endif 

#endif // PARLIB_ARCH_H
