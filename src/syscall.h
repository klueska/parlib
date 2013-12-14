/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/
/* Andrew Waterman <waterman@cs.berkeley.edu>	*/

#ifndef __PARLIB_SYSCALL_H__
#define __PARLIB_SYSCALL_H__

struct uthread;

// Akaros event compatibility layer
struct syscall {
  void *u_data;
};
struct event_msg {
  void *ev_arg3;
  struct syscall sysc;
};

#define EV_SYSCALL 0
#define MAX_NR_EVENT 1
typedef void (*handle_event_t)(struct event_msg *ev_msg, unsigned ev_type);
extern handle_event_t ev_handlers[MAX_NR_EVENT];

#endif // __PARLIB_SYSCALL_H__
