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
#define _PARLIB_CONTEXT_H	1

/* Get machine dependent definition of data structures.  */
#include <ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get user context and store it in variable pointed to by UCP.  */
extern int parlib_getcontext (ucontext_t *__ucp) __THROWNL;

/* Set user context from information of variable pointed to by UCP.  */
extern int parlib_setcontext (__const ucontext_t *__ucp) __THROWNL;

/* Save current context in context variable pointed to by OUCP and set
   context from variable pointed to by UCP.  */
extern int parlib_swapcontext (ucontext_t *__restrict __oucp,
			__const ucontext_t *__restrict __ucp) __THROWNL;

/* Manipulate user context UCP to continue with calling functions FUNC
   and the ARGC-1 parameters following ARGC when the context is used
   the next time in `setcontext' or `swapcontext'.

   We cannot say anything about the parameters FUNC takes; `void'
   is as good as any other choice.  */
#define parlib_makecontext(__ucp, __func, __argc, ...) \
  makecontext(__ucp, __func, __argc, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* context.h */
