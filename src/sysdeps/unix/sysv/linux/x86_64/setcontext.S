/* Install given context.
   Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
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

/* Modified from glibc's setcontext for our custom user_context.

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

/* void parlib_setcontext (struct user_context *ucp) */
HIDDEN_ENTRY(parlib_setcontext)
	/* Restore the floating point state. */
	fldcw oFPUCW(%rdi)
	ldmxcsr oMXCSR(%rdi)

	/* Load the new stack pointer, and the callee saved registers. */
	cfi_def_cfa(%rdi, 0)
	cfi_offset(%rbx,oRBX)
	cfi_offset(%rbp,oRBP)
	cfi_offset(%r12,oR12)
	cfi_offset(%r13,oR13)
	cfi_offset(%r14,oR14)
	cfi_offset(%r15,oR15)
	cfi_offset(%rsp,oRSP)
	cfi_offset(%rip,oRIP)

	movq oRSP(%rdi), %rsp
	movq oRBX(%rdi), %rbx
	movq oRBP(%rdi), %rbp
	movq oR12(%rdi), %r12
	movq oR13(%rdi), %r13
	movq oR14(%rdi), %r14
	movq oR15(%rdi), %r15

	/* The following ret should return to the address set with
	getcontext.  Therefore push the address on the stack.  */
	movq	oRIP(%rdi), %rcx
	pushq	%rcx

	/* End FDE here, we fall into another context.  */
	cfi_endproc
	cfi_startproc

	/* Return */
	ret
PSEUDO_END(parlib_setcontext)

#undef parlib_setcontext
weak_alias (INTERNAL(parlib_setcontext), parlib_setcontext)
