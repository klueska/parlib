/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/
/* Andrew Waterman <waterman@cs.berkeley.edu>	*/

#ifndef __PARLIB_INTERNAL_SYSCALL_H__
#define __PARLIB_INTERNAL_SYSCALL_H__

#include <unistd.h>

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
#include "../syscall.h"
#include "parlib.h"
#include "pthread_pool.h"
#include <sys/mman.h>

typedef struct {
  void *(*func)(void*);
  struct event_msg ev_msg;
} yield_callback_arg_t;

#define uthread_blocking_call(__func, ...) \
({ \
  typeof(__func(__VA_ARGS__)) ret; \
  void *do_##__func(void *arg) { \
    ret = __func(__VA_ARGS__); \
    handle_event_t handler = ev_handlers[EV_SYSCALL]; \
    handler((struct event_msg*)arg, EV_SYSCALL); \
    return NULL; \
  } \
  yield_callback_arg_t arg; \
  arg.func = &do_##__func; \
  uthread_yield(true, __uthread_yield_callback, &arg); \
  ret; \
})

static void __uthread_yield_callback(struct uthread *uthread, void *__arg) {
  assert(sched_ops->thread_blockon_sysc);

  yield_callback_arg_t *arg = (yield_callback_arg_t*)__arg;
  arg->ev_msg.ev_arg3 = &arg->ev_msg.sysc;
  uthread->sysc = &arg->ev_msg.sysc;

  sched_ops->thread_blockon_sysc(uthread, &arg->ev_msg.sysc);

  pooled_pthread_start(arg->func, &arg->ev_msg);
}

#endif // __PARLIB_INTERNAL_SYSCALL_H__
