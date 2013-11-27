/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/
/* Andrew Waterman <waterman@cs.berkeley.edu>	*/

#ifndef __ASYNC_WRAPPERS__
#define __ASYNC_WRAPPERS__

struct uthread;

extern void (*async_syscall_start)(struct uthread*);
extern void (*async_syscall_done)(struct uthread*);

#endif // __ASYNC_WRAPPERS__

