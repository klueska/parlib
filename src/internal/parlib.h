#ifndef PARLIB_INTERNAL_ASSERT_H
#define PARLIB_INTERNAL_ASSERT_H

#ifndef __ASSEMBLER__

#include <assert.h>
#include "../export.h"

//#define PARLIB_DEBUG
#ifndef PARLIB_DEBUG
# undef assert
# define assert(x) (__builtin_constant_p(x) && (x) == 0 ? __builtin_unreachable() : (x))
#endif

/* TLS variables used by the pthread backing each uthread. */
struct backing_pthread {
	int futex;
	void *(*syscall) (void*);
	void *arg;
};
extern __thread struct backing_pthread __backing_pthread TLS_INITIAL_EXEC;

/* When we wake up in the backing thread, we need to know what to do.
 * Traditionally, we have just exited, but now we also want to use this thread
 * to simulate async I/O for the thread. */
enum {
	BACKING_THREAD_SLEEP,
	BACKING_THREAD_SYSCALL,
	BACKING_THREAD_EXIT,
};

#endif // __ASSEMBLER__

#define INTERNAL(name) plt_bypass_ ## name

#endif // PARLIB_INTERNAL_ASSERT_H
