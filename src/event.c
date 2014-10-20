/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/

#include "internal/parlib.h"
#include <sys/queue.h>
#include <malloc.h>
#include "vcore.h"
#include "event.h"
#include "spinlock.h"

struct pvc_event_msg {
	struct event_msg *ev_msg;
	unsigned ev_type;
	STAILQ_ENTRY(pvc_event_msg) next;
};
STAILQ_HEAD(pvc_event_queue, pvc_event_msg);

struct pvc_event_queue *pvc_event_queue;
spinlock_t *pvc_lock;

void event_lib_init()
{
	pvc_event_queue = malloc(sizeof(struct pvc_event_queue) * max_vcores());
	pvc_lock = malloc(sizeof(spinlock_t) * max_vcores());
	for (int i=0; i<max_vcores(); i++) {
		STAILQ_INIT(&pvc_event_queue[i]);
		spinlock_init(&pvc_lock[i]);
	}
}

void send_event(struct event_msg *ev_msg, unsigned ev_type, int vcoreid)
{
	struct pvc_event_msg *m = malloc(sizeof(struct pvc_event_msg));
	m->ev_msg = ev_msg;
	m->ev_type = ev_type;
	spinlock_lock(&pvc_lock[vcoreid]);
	STAILQ_INSERT_TAIL(&pvc_event_queue[vcoreid], m, next);
	spinlock_unlock(&pvc_lock[vcoreid]);
	vcore_signal(vcoreid);
}

void handle_events()
{
	struct pvc_event_msg *m;
	int vcoreid = vcore_id();
	spinlock_lock(&pvc_lock[vcoreid]);
	while((m = STAILQ_FIRST(&pvc_event_queue[vcoreid]))) {
		STAILQ_REMOVE_HEAD(&pvc_event_queue[vcoreid], next);
		spinlock_unlock(&pvc_lock[vcoreid]);

		handle_event_t handler = ev_handlers[m->ev_type];
		handler(m->ev_msg, m->ev_type);

		spinlock_lock(&pvc_lock[vcoreid]);
	}
	spinlock_unlock(&pvc_lock[vcoreid]);
}

