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

#include <stddef.h>
#include <sys/syscall.h>

#include "internal/vcore.h"
#include "internal/tls.h"
#include "internal/assert.h"
#include "atomic.h"
#include "tls.h"
#include "vcore.h"

/* Reference to the main thread's tls descriptor */
void *main_tls_desc = NULL;

/* Current tls_desc for each running vcore, used when swapping contexts onto a vcore */
__thread void *current_tls_desc = NULL;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void)
{
	extern void *_dl_allocate_tls(void *mem) internal_function;
	void *tcb = _dl_allocate_tls(NULL);
	if (!tcb) {
		fprintf(stderr, "could not allocate tls!\n");
		abort();
	}
	/* Make sure the TLS is set up properly - its tcb pointer 
	 * points to itself. */
	tcbhead_t *head = (tcbhead_t*)tcb;
	size_t offset = offsetof(tcbhead_t, multiple_threads);

	/* These fields in the tls_desc need to be set up for linux to work
	 * properly with TLS. Take a look at how things are done in libc in the
	 * nptl code for reference. */
	memcpy(tcb+offset, main_tls_desc+offset, sizeof(tcbhead_t)-offset);
	head->tcb = tcb;
	head->self = tcb;
	head->multiple_threads = true;
	return tcb;
}

/* Reinitialize / reset / refresh a TLS to its initial values.  This doesn't do
 * it properly yet, it merely frees and re-allocates the TLS, which is why we're
 * slightly ghetto and return the pointer you should use for the TCB. */
void *reinit_tls(void *tcb)
{
    free_tls(tcb);
    return allocate_tls();
}

/* Free a previously allocated TLS region */
void free_tls(void *tcb)
{
	extern void _dl_deallocate_tls (void *tcb, bool dealloc_tcb) internal_function;
	assert(tcb);
	_dl_deallocate_tls(tcb, 1);
}

/* Constructor to get a reference to the main thread's TLS descriptor */
int tls_lib_init()
{
	/* Make sure this only runs once */
	static bool initialized = false;
	if (initialized)
	    return 0;
	initialized = true;
	
	/* Get a reference to the main program's TLS descriptor */
	current_tls_desc = get_current_tls_base();
	main_tls_desc = current_tls_desc;
	return 0;
}

/* Initialize tls for use in this vcore */
void init_tls(uint32_t vcoreid)
{
	/* Get a reference to the current TLS region in the GDT */
	void *tcb = get_current_tls_base();
	assert(tcb);

  init_arch_tls_data(&__vcores[vcoreid].arch_tls_data, tcb, vcoreid);

	/* Set the tls_desc in the tls_desc array */
	__vcore_tls_descs[vcoreid] = tcb;
}

/* Passing in the vcoreid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t vcoreid)
{
  assert(tls_desc != NULL);

  set_current_tls_base(tls_desc, &__vcores[vcoreid].arch_tls_data);

  extern __thread int __vcore_id;
  current_tls_desc = tls_desc;
  __vcore_id = vcoreid;
}

/* Get the tls descriptor currently set for a given vcore. This should
 * only ever be called once the vcore has been initialized */
void *get_tls_desc(uint32_t vcoreid)
{
  void *desc = TLS_DESC(__vcores[vcoreid].arch_tls_data);
  assert(desc);
  return desc;
}

