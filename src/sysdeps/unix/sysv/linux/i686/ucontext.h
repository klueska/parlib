/*
 * Copyright (c) 2014 The Regents of the University of California
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

#ifndef PARLIB_UCONTEXT_H
#define PARLIB_UCONTEXT_H 

#include_next <ucontext.h>
#define user_context ucontext

static inline void parlib_makecontext(struct user_context *ucp,
                                      void (*entry)(void),
                                      void *stack_bottom,
                                      uint32_t stack_size)
{
	extern void parlib_getcontext(struct user_context *__ucp);
	parlib_getcontext(ucp);
	ucp->uc_stack.ss_sp = stack_bottom;
	ucp->uc_stack.ss_size = stack_size;
	makecontext(ucp, entry, 0);
}

#endif // PARLIB_UCONTEXT_H
