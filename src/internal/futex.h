/*
 * Copyright (c) 2011 The Regents of the University of California
 * Benjamin Hindman <benh@cs.berkeley.edu>
 *
 * This file is part of Parlib.
 *
 * The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef FUTEX_H
#define FUTEX_H

#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __linux__
#error "expecting __linux__ (for now, this library only runs on Linux)"
#endif

#include <linux/futex.h>
#include <sys/syscall.h>


inline static void futex_wait(void *futex, int comparand)
{
  while (*(int*)futex == comparand) {
    syscall(SYS_futex, futex, FUTEX_WAIT, comparand, NULL, NULL, 0);
  }
}


inline static void futex_wakeup_one(void *futex)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 0);
  if (!(r == 0 || r == 1)) {
    fprintf(stderr, "futex: futex_wakeup_one failed");
    exit(1);
  }
}


inline static void futex_wakeup_all(void *futex)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
  if (r < 0) {
    fprintf(stderr, "futex: futex_wakeup_all failed");
    exit(1);
  }
}


#endif /* FUTEX_H */
