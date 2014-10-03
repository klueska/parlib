/* Offsets and other constants needed in the *context() function
   implementation for Linux/x86-64.
   Copyright (C) 2002 Free Software Foundation, Inc.
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

/* My own defines for stuff found in the *context.S files */
#define ENTRY(__func) \
  .globl __func; .type __func,@function; .align 1<<4; __func: cfi_startproc;
#define HIDDEN_ENTRY(__func) \
  ENTRY(__func); .hidden __func;
#define SYSCALL_ERROR() \
  movq __libc_errno@GOTTPOFF(%rip), %rcx; xorl %edx, %edx; subq %rax, %rdx; movl %edx, %fs:(%rcx); orq $-1, %rax; jmp L(pseudo_end); 
#define PSEUDO_END(__func) \
 cfi_endproc; .size __func,.-##__func;
#define weak_alias(__orig, __func) \
  .weak __func ; __func = __orig
#define L(__name) .L##__name
#define cfi_adjust_cfa_offset(__value) \
  .cfi_adjust_cfa_offset __value
#define cfi_def_cfa(__reg, __value) \
  .cfi_def_cfa __reg, __value
#define cfi_offset(__reg, __value) \
  .cfi_offset __reg, __value
#define cfi_startproc .cfi_startproc
#define cfi_endproc   .cfi_endproc

/* Signal mask defines */
#define SIG_BLOCK   0
#define SIG_SETMASK 2

#define _NSIG8               8
#define __NR_rt_sigprocmask 14

/* Offsets of the fields in the ucontext_t structure.  */
#define oRBX        128
#define oRBP        120
#define oR12         72
#define oR13         80
#define oR14         88
#define oR15         96
#define oRDI        104
#define oRSI        112
#define oRDX        136
#define oRCX        152
#define oR8          40
#define oR9          48
#define oRIP        168
#define oRSP        160
#define oFPREGSMEM  424
#define oFPREGS     224
#define oMXCSR      448
#define oSIGMASK    296

