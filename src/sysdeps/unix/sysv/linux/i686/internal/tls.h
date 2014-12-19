/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * Andrew Waterman <waterman@cs.berkeley.edu>
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

#ifndef PARLIB_I386_TLS_H
#define PARLIB_I386_TLS_H

#include <unistd.h>
#include <sys/syscall.h>
#include <asm/ldt.h>

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

/* This is the default entry number of the TLS segment in the GDT when a
 * process first starts up. */
#define DEFAULT_TLS_GDT_ENTRY 6

/* Linux fills in 5 LDT entries, but effectively only uses 2.  We'll assume 5
 * are reserved, just to be safe */
#define RESERVED_LDT_ENTRIES 5

#define arch_tls_data_t struct user_desc
#define TLS_DESC(tls_data) ((void *)(unsigned long)((tls_data).base_addr))
#define CLONE_TLS_PARAM(tls_data) (&(tls_data))

/* Get the current tls base address */
static __inline void *get_current_tls_base()
{
  struct user_desc ud;
  ud.entry_number = DEFAULT_TLS_GDT_ENTRY;
  int ret = syscall(SYS_get_thread_area, &ud);
  assert(ret == 0);
  assert(ud.base_addr);
  return (void *)(unsigned long)ud.base_addr;
}

/* Set the current tls base address */
static __inline void set_current_tls_base(void *tls_desc,
                                          arch_tls_data_t *data)
{
  if (data == NULL) {
    struct user_desc ud;
    ud.entry_number = DEFAULT_TLS_GDT_ENTRY;
    int ret = syscall(SYS_get_thread_area, &ud);
    assert(ret == 0);
    ud.base_addr = (uintptr_t)tls_desc;
    ret = syscall(SYS_set_thread_area, &ud);
    assert(ret == 0);
  } else {
    data->base_addr = (unsigned int)tls_desc;
    int ret = syscall(SYS_modify_ldt, 1, data, sizeof(struct user_desc));
    assert(ret == 0);
    TLS_SET_SEGMENT_REGISTER(data->entry_number, 1);
  }
}

static void init_arch_tls_data(arch_tls_data_t *data, void *tls_desc,
                               uint32_t vcoreid)
{
	/* Set up the TLS region as an entry in the LDT */
	memset(data, 0, sizeof(struct user_desc));
	data->entry_number = vcoreid + RESERVED_LDT_ENTRIES;
	data->limit = (-1);
	data->seg_32bit = 1;
	data->limit_in_pages = 1;
	data->useable = 1;
}

#endif /* PARLIB_I386_TLS_H */
