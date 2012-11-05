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

#ifndef HT_TLS_H
#define HT_TLS_H

#include <stdint.h>
#include <stdlib.h>

#ifndef __GNUC__
  #error "You need to be using gcc to compile this library..."
#endif 

#ifdef __cplusplus
extern "C" {
#endif

/* Declaration of types needed for dynamically allocatable tls */
struct dtls_key;
typedef struct dtls_key dtls_key_t;
struct dtls_list;
typedef struct dtls_list dtls_list_t;
typedef void (*dtls_dtor_t)(void*);

/* Reference to the main thread's tls descriptor */
extern void *main_tls_desc;

/* Current tls_desc for each running vcore, used when swapping uthreads onto a
 * vcore */
extern __thread void *current_tls_desc;

/* A pointer to a list of dynamically allocated tls regions */
extern __thread dtls_list_t *current_dtls_list;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void);

/* Reinitialize an already allocated TLS, returns 0 on failure.  */
void *reinit_tls(void *tcb);

/* Free a previously allocated TLS region */
void free_tls(void *tcb);

/* Set the tls descriptor on the current uthread or vcore. Passing in the
 * vcoreid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t vcoreid);

/* Get the tls descriptor currently set for a given vcore. This should
 * only ever be called once the vcore has been initialized */
void *get_tls_desc(uint32_t vcoreid);

/* Initialize a dtls_key for dynamically setting/getting uthread local storage
 * on a uthread or vcore. */
dtls_key_t *dtls_key_create(dtls_dtor_t dtor);

/* Destroy a dtls key. */
void dtls_key_delete(dtls_key_t *key);

/* Set dtls storage for the provided dtls key on the current uthread or vcore. */
void set_dtls(dtls_key_t *key, void *dtls);

/* Get dtls storage for the provided dtls key on the current uthread or vcore. */
void *get_dtls(dtls_key_t *key);

/* Destroy all dtls storage associated with the current uthread or vcore. */
void destroy_dtls();

#ifndef __PIC__

#define begin_safe_access_tls_vars()

#define end_safe_access_tls_vars()

#else

#include <features.h>
#if __GNUC_PREREQ(4,4)

#define begin_safe_access_tls_vars()                                    \
  void __attribute__((noinline, optimize("O0")))                        \
  safe_access_tls_var_internal() {                                      \
    asm("");                                                            \

#define end_safe_access_tls_vars()                                      \
  } safe_access_tls_var_internal();                                     \

#else

#define begin_safe_access_tls_vars()                                                   \
  printf("ERROR: For PIC you must be using gcc 4.4 or above for tls support!\n");      \
  printf("ERROR: As a quick fix, try recompiling your application with -static...\n"); \
  exit(2);

#define end_safe_access_tls_vars()                                         \
  printf("Will never be called because we abort above!");                  \
  exit(2);

#endif //__GNUC_PREREQ
#endif // __PIC__

#define begin_access_tls_vars(tls_desc)                           \
	int vcoreid = vcore_id();                                     \
	void *temp_tls_desc = current_tls_desc;                       \
	cmb();                                                        \
	set_tls_desc(tls_desc, vcoreid);                              \
	begin_safe_access_tls_vars();

#define end_access_tls_vars()                                     \
	end_safe_access_tls_vars();                                   \
	set_tls_desc(temp_tls_desc, vcoreid);                         \
	cmb();

#define safe_set_tls_var(name, val)                               \
({                                                                \
	begin_safe_access_tls_vars();                                 \
	name = val;                                                   \
	end_safe_access_tls_vars();                                   \
})

#define safe_get_tls_var(name)                                    \
({                                                                \
	typeof(name) __val;                                           \
	begin_safe_access_tls_vars();                                 \
	__val = name;                                                 \
	end_safe_access_tls_vars();                                   \
	__val;                                                        \
})

#ifdef __cplusplus
}
#endif

#endif /* HT_TLS_H */
