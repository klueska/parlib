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

/**
 * Vcore interface.
 */

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __ELF__
# error "expecting ELF file format"
#endif

#ifndef VCORE_H
#define VCORE_H

#define LOG2_MAX_VCORES 6
#define MAX_VCORES (1 << LOG2_MAX_VCORES)
#define VCORE_UNMAPPED (-1)

#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "tls.h"
#include "atomic.h"
#include "parlib-config.h"
#include "context.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef COMPILING_PARLIB
# define vcore_lib_init INTERNAL(vcore_lib_init)
# define vcore_request INTERNAL(vcore_request)
# define vcore_reenter INTERNAL(vcore_reenter)
# define clear_notif_pending INTERNAL(clear_notif_pending)
# define enable_notifs INTERNAL(enable_notifs)
# define disable_notifs INTERNAL(disable_notifs)
#endif

/* The vcore type */
struct vcore;
typedef struct vcore vcore_t;

/**
 *  Array of pointers to TLS descriptors for each vcore.
 */
extern void **vcore_tls_descs;

/**
 * Array of mappings from vcore id to pcore id.
 */
extern int *vcore_map;

/**
 * Per vcore context for entry at the top of the vcore stack.
 */
extern __thread ucontext_t vcore_entry_context TLS_INITIAL_EXEC;

/**
 * Current user context running on each vcore, used when interrupting a
 * user context because of async I/O or signal handling. Hard Thread 0's
 * vcore_saved_ucontext is initialized to the continuation of the main thread's
 * context the first time it's vcore_entry() function is invoked.
 */
extern __thread ucontext_t *vcore_saved_ucontext TLS_INITIAL_EXEC;

/**
 * Current tls_desc of the user context running on each vcore, used when
 * interrupting a user context because of async I/O or signal handling. Hard
 * Thread 0's vcore_saved_tls_desc is initialized to the tls_desc of the main
 * thread's context the first time it's vcore_entry() function is invoked.
 */
extern __thread void *vcore_saved_tls_desc TLS_INITIAL_EXEC;

/**
 * User defined entry point for each vcore.  If vcore_saved_ucontext is
 * set, this function should most likely just restore it, otherwise, go on from
 * there.
 */
extern void vcore_entry();

/* Function for sending a signal to a vcore. */
extern void vcore_signal(int vcoreid);

/* Callback to be implemented when a signal is recieved on a vcore. */
extern void vcore_sigentry();

/* Initialization routine for the vcore subsystem. */
int vcore_lib_init();

/**
 * Function to reenter a vcore at the top of its stack stack.
 */
void vcore_reenter(void (*entry_func)(void));

/**
 * Requests k additional vcores. Returns -1 if the request is impossible.
 * Otherwise, blocks calling vcore until the request is granted and returns 0.
*/
extern int vcore_request(int k);

/**
 * Relinquishes the calling vcore.
*/
extern void vcore_yield();

/**
 * Returns the id of the calling vcore.
 */
static inline int vcore_id(void)
{
	extern __thread int __vcore_id TLS_INITIAL_EXEC;
	return __vcore_id;
}

/**
 * Returns the current number of vcores allocated.
 */
static inline size_t num_vcores(void)
{
	extern atomic_t __num_vcores;
	return atomic_read(&__num_vcores);
}

/**
 * Returns the maximum number of allocatable vcores.
 */
static inline size_t max_vcores(void)
{
	extern volatile int __max_vcores;
	return MIN(__max_vcores, MAX_VCORES);
}

/**
 * Returns whether you are currently running in vcore context or not.
 */
static inline bool in_vcore_context() {
	extern __thread bool __in_vcore_context TLS_INITIAL_EXEC;
	return __in_vcore_context;
}

/**
 * Clears the flag for pending notifications
 */
void clear_notif_pending(uint32_t vcoreid);

/**
 * Enable Notifications
 */
void enable_notifs(uint32_t vcoreid);

/**
 * Disable Notifications
 */
void disable_notifs(uint32_t vcoreid);

#ifndef PARLIB_NO_UTHREAD_TLS
  #define vcore_begin_access_tls_vars(vcore_id) \
    begin_access_tls_vars(vcore_tls_descs[(vcore_id)])

  #define vcore_end_access_tls_vars() \
    end_access_tls_vars()

  #define vcore_set_tls_var(name, val)                                 \
  {                                                                    \
  	typeof(val) __val = val;                                           \
  	begin_access_tls_vars(vcore_tls_descs[vcore_id()]);              \
  	name = __val;                                                      \
  	end_access_tls_vars();                                             \
  }
  
  #define vcore_get_tls_var(name)                                      \
  ({                                                                   \
  	typeof(name) val;                                                  \
  	begin_access_tls_vars(vcore_tls_descs[vcore_id()]);              \
  	val = name;                                                        \
  	end_access_tls_vars();                                             \
  	val;                                                               \
  })
#else
  #define vcore_begin_access_tls_vars(vcore_id)
  #define vcore_end_access_tls_vars()

  #define vcore_set_tls_var(name, val)                                 \
  	name = val;                                                        \
  
  #define vcore_get_tls_var(name)                                      \
  	name
#endif

#ifdef __cplusplus
}
#endif

#endif // VCORE_H
