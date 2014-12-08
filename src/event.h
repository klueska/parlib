/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/
/* Andrew Waterman <waterman@cs.berkeley.edu>	*/

#ifndef PARLIB_EVENT_H
#define PARLIB_EVENT_H

#ifdef COMPILING_PARLIB
# define event_lib_init INTERNAL(event_lib_init)
# define send_event INTERNAL(send_event)
# define handle_events INTERNAL(handle_events)
# define enable_notifs INTERNAL(enable_notifs)
# define disable_notifs INTERNAL(disable_notifs)
#endif

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

void event_lib_init();
void send_event(struct event_msg *ev_msg, unsigned ev_type, int vcoreid);
void handle_events();

void clear_notif_pending(uint32_t vcoreid);
void enable_notifs(uint32_t vcoreid);
void disable_notifs(uint32_t vcoreid);

/* This call only exists in the linux port of parlib.  In Akaros, timeouts can
 * be set on syscalls using the normal alarm stuff, but this would be
 * prohibitively slow on linux with our current alarm implementation. */
void EXPORT_SYMBOL set_syscall_timeout(uint64_t timeout_usec);

#endif // PARLIB_EVENT_H
