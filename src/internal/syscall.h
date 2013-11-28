/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/
/* Andrew Waterman <waterman@cs.berkeley.edu>	*/

#ifndef __INTERNAL_ASYNC_WRAPPERS__
#define __INTERNAL_ASYNC_WRAPPERS__

#ifdef __GLIBC__
#define __SUPPORTED_C_LIBRARY__
#define __internal_open __open
#define __internal_close __close
#define __internal_read __read
#define __internal_write __write
int __open(const char*, int, ...);
int __close(int);
ssize_t __read(int, void*, size_t);
ssize_t __write(int, const void*, size_t);
#endif

#include "../uthread.h"
#include "pthread_pool.h"
#include <sys/mman.h>

#define uthread_blocking_call(__func, ...) \
({ \
  typeof(__func(__VA_ARGS__)) ret; \
  void *do_##__func(void* uthread) { \
    ret = __func(__VA_ARGS__); \
    printf("uthread in do func %p\n", uthread); \
    async_syscall_done((struct uthread*)uthread); \
    return NULL; \
  } \
  uthread_yield(true, __uthread_yield_callback, &do_##__func); \
  ret; \
})

static void __uthread_yield_callback(struct uthread *uthread, void *func) {
  printf("uthread in yield cb %p\n", uthread);
  async_syscall_start(uthread);
  pooled_pthread_start(func, uthread);
}

#endif // __INTERNAL_ASYNC_WRAPPERS__
