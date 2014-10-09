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

#include "context.h"
typedef struct ucontext uthread_context_t;

#ifdef __i386__
  #define internal_function __attribute ((regparm (3), stdcall))
#elif __x86_64__
  #define internal_function
#endif

#define PGSIZE getpagesize()
#define ARCH_CL_SIZE 64

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

#define init_uthread_stack_ARCH(uth, stack_top, size)   \
{                                                       \
	ucontext_t *uc = &(uth)->uc;                        \
	parlib_getcontext(uc);                              \
	uc->uc_stack.ss_sp = (void*)(stack_top);            \
	uc->uc_stack.ss_size = (uint32_t)(size);            \
}

#define init_uthread_entry_ARCH(uth, entry) \
  parlib_makecontext(&(uth)->uc, (entry), 0)

#else // !__linux__
  #error "Your architecture is not yet supported by parlib!"
#endif 

#endif // PARLIB_ARCH_H
