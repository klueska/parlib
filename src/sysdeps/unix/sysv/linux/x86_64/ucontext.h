/* System V ABI compliant user-level context switching support.  */

#ifndef PARLIB_UCONTEXT_H
#define PARLIB_UCONTEXT_H

#include "common.h"

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

/* Initialize a new user context with an engtry function */
static inline void parlib_makecontext(struct user_context *ucp,
                                      void (*entry)(void),
                                      void *stack_bottom,
                                      uint32_t stack_size)
{
	/* Stack pointers in a fresh stackframe need to be such that adding or
	 * subtracting 8 will result in 16 byte alignment (AMD64 ABI).  The reason
	 * is so that input arguments (on the stack) are 16 byte aligned.  The
	 * extra 8 bytes is the retaddr, pushed on the stack.  Compilers know they
	 * can subtract 8 to get 16 byte alignment for instructions like movaps. */
	ucp->tf_rsp = ROUNDDOWN((((uintptr_t)stack_bottom) + stack_size), 16) - 8;
	ucp->tf_rip = (uint64_t) entry;
	ucp->tf_rbp = 0;  /* for potential backtraces */
	/* No need to bother with setting the other GP registers; the called
	 * function won't care about their contents. */
	ucp->tf_mxcsr = 0x00001f80; /* x86 default mxcsr */
	ucp->tf_fpucw = 0x037f;   /* x86 default FP CW */
}

#endif // PARLIB_UCONTEXT_H
