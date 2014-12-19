/* Save current context.
   Copyright (C) 2002, 2005, 2011 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>, 2002.

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

/* Modified from glibc's getcontext for our custom user_context.

struct user_context {
    uint64_t tf_rbx;
    uint64_t tf_rbp;
    uint64_t tf_r12;
    uint64_t tf_r13;
    uint64_t tf_r14;
    uint64_t tf_r15;
    uint64_t tf_rip;
    uint64_t tf_rsp;
    uint32_t tf_mxcsr;
    uint16_t tf_fpucw;
} __attribute__((aligned(ARCH_CL_SIZE)));
*/

#include "internal/asm.h"
#include "internal/parlib.h"
#include "context.h"
#include "export.h"

/*  void parlib_getcontext(struct user_context *ucp) */
HIDDEN_ENTRY(parlib_getcontext)
	/* Save the callee-saved registers */
	movq %rbx, oRBX(%rdi)
	movq %rbp, oRBP(%rdi)
	movq %r12, oR12(%rdi)
	movq %r13, oR13(%rdi)
	movq %r14, oR14(%rdi)
	movq %r15, oR15(%rdi)

	/* Save the rip and rsp of the next instruction in the calling frame. */
	movq (%rsp), %rcx
	movq %rcx, oRIP(%rdi)
	leaq 8(%rsp), %rcx    /* Exclude the return address.  */
	movq %rcx, oRSP(%rdi)

	/* Save the necessary floating-point state.  */
	stmxcsr oMXCSR(%rdi)
	fnstcw oFPUCW(%rdi)

	/* Return */
	ret
PSEUDO_END(parlib_getcontext)

#undef parlib_getcontext
weak_alias (INTERNAL(parlib_getcontext), parlib_getcontext)
