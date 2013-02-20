/* Offsets and other constants needed in the *context() function
   implementation for Linux/i686
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
#define ENTRY(func) \
 .globl func; .type func##,@function; .align 1<<4; func##: cfi_startproc;
#define cfi_adjust_cfa_offset(arg) \
  .cfi_adjust_cfa_offset arg
#define cfi_rel_offset(arg0, arg1) \
  .cfi_rel_offset arg0, arg1
#define ENTER_KERNEL \
  call *%gs:0x10
#define cfi_adjust_cfa_offset(arg) \
  .cfi_adjust_cfa_offset arg
#define cfi_restore(arg) \
  .cfi_restore arg
#define cfi_def_cfa(arg0, arg1) \
  .cfi_def_cfa arg0, arg1
#define cfi_offset(arg0, arg1) \
  .cfi_offset arg0, arg1
#define cfi_endproc \
  .cfi_endproc
#define cfi_startproc \
  .cfi_startproc
#define L(func) \
  .L##func
#define PSEUDO_END(func) \
  cfi_endproc; .size func##,.-##func;
#define weak_alias(func, alias) \
.weak alias ; alias = func

#define SYSCALL_ERROR_LABEL __syscall_error
#define __NR_sigprocmask 126

/* Signal mask defines */
#define SIG_BLOCK 0
#define SIG_SETMASK 2

/* Offsets of the fields in the ucontext_t structure.  */
#define oLINK 4
#define oSS_SP 8
#define oSS_SIZE 16
#define oGS 20
#define oFS 24
#define oEDI 36
#define oESI 40
#define oEBP 44
#define oESP 48
#define oEBX 52
#define oEDX 56
#define oECX 60
#define oEAX 64
#define oEIP 76
#define oFPREGS 96
#define oSIGMASK 108
#define oFPREGSMEM 236

