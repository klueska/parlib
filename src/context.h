/* Copyright (C) 1997, 1998, 1999, 2000, 2011 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* System V ABI compliant user-level context switching support.  */

#ifndef _PARLIB_CONTEXT_H
#define _PARLIB_CONTEXT_H

#ifdef COMPILING_PARLIB
# include "internal/parlib.h"
# define parlib_getcontext INTERNAL(parlib_getcontext)
# define parlib_setcontext INTERNAL(parlib_setcontext)
# define parlib_swapcontext INTERNAL(parlib_swapcontext)
#endif

#ifndef __ASSEMBLER__

/* Get machine dependent definition of data structures.  */
#include "arch.h"
#include "ucontext.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Defined in architecture specific ucontext.h
static inline void parlib_makecontext(struct user_context *ucp,
                                      void (*entry)(void),
                                      void *stack_bottom,
                                      size_t stack_size)
*/

/* The following MUST be function calls (i.e. not inlined functions), otherwise
   the compiler might play tricks on us and we will save / restore stuff
   incorrectly. */

/* Get user context and store it in variable pointed to by UCP.  */
extern void parlib_getcontext(struct user_context *__ucp);

/* Set user context from information of variable pointed to by UCP.  */
extern void parlib_setcontext(struct user_context *__ucp);

/* Save current context in context variable pointed to by OUCP and set
   context from variable pointed to by NUCP.  */
extern void parlib_swapcontext(struct user_context *__oucp,
                               struct user_context *__nucp);

#ifdef __cplusplus
}
#endif

#endif

#endif /* context.h */
