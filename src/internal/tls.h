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

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "arch.h"
#include "glibc-tls.h"
#include "vcore.h"

#ifdef __x86_64__
  #include <asm/prctl.h>
  #include <sys/prctl.h>
  int arch_prctl(int code, unsigned long *addr);
#endif 

/* This is the default entry number of the TLS segment in the GDT when a
 * process first starts up. */
#define DEFAULT_TLS_GDT_ENTRY 6

/* Linux fills in 5 LDT entries, but effectively only uses 2.  We'll assume 5
 * are reserved, just to be safe */
#define RESERVED_LDT_ENTRIES 5

/* Initialize the tls subsystem for use */
int tls_lib_init();

/* Initialize tls for use by a vcore */
void init_tls(uint32_t vcoreid);

/* Get the current tls base address */
static __inline void *get_current_tls_base()
{
	size_t addr;
#ifdef __i386__
    struct user_desc ud;
	ud.entry_number = DEFAULT_TLS_GDT_ENTRY;
	int ret = syscall(SYS_get_thread_area, &ud);
	assert(ret == 0);
	assert(ud.base_addr);
    addr = (size_t)ud.base_addr;
#elif __x86_64__
	arch_prctl(ARCH_GET_FS, &addr);
#endif
	return (void *)addr;
}

#ifdef __i386__
  #define TLS_SET_SEGMENT_REGISTER(entry, ldt) \
    asm volatile("movl %0, %%gs" :: "r" ((entry<<3)|(0x4&(ldt<<2))|0x3) : "memory")

  #define TLS_GET_SEGMENT_REGISTER(entry, ldt, perms) \
  ({ \
    int reg = 0; \
    asm volatile("movl %%gs, %0" : "=r" (reg)); \
    *(entry) = (reg >> 3); \
    *(ldt) = (0x1 & (reg >> 2)); \
    *(perms) = (0x3 & reg); \
  })

#elif __x86_64
  #define TLS_SET_SEGMENT_REGISTER(entry, ldt) \
    asm volatile("movl %0, %%fs" :: "r" ((entry<<3)|(0x4&(ldt<<2))|0x3) : "memory")

  #define TLS_GET_SEGMENT_REGISTER(entry, ldt, perms) \
  ({ \
    int reg = 0; \
    asm volatile("movl %%fs, %0" : "=r" (reg)); \
    *(entry) = (reg >> 3); \
    *(ldt) = (0x1 & (reg >> 2)); \
    *(perms) = (0x3 & reg); \
  })

#endif

#endif /* PARLIB_TLS_INTERNAL_H */

