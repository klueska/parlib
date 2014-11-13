/* See COPYING.LESSER for copyright information. */
/* Kevin Klues <klueska@cs.berkeley.edu>	*/

#include "internal/parlib.h"
#include <sys/queue.h>
#include <stdlib.h>
#include "parlib.h"
#include "vcore.h"
#include "event.h"
#include "spinlock.h"
#include "atomic.h"

struct pvc_event_msg {
	struct event_msg *ev_msg;
	unsigned ev_type;
	STAILQ_ENTRY(pvc_event_msg) next;
};
STAILQ_HEAD(pvc_event_queue, pvc_event_msg);

struct vc_mgmt {
	struct pvc_event_queue evq;
	spinlock_t evq_lock;
	atomic_t notifs_enabled;
	atomic_t notif_pending;
} __attribute__((aligned(ARCH_CL_SIZE)));

static struct vc_mgmt *vc_mgmt;

void event_lib_init()
{
	vc_mgmt = parlib_aligned_alloc(PGSIZE,
	            sizeof(struct vc_mgmt) * max_vcores());
	for (int i=0; i<max_vcores(); i++) {
		STAILQ_INIT(&(vc_mgmt[i].evq));
		spinlock_init(&(vc_mgmt[i].evq_lock));
		vc_mgmt[i].notifs_enabled = ATOMIC_INITIALIZER(1);
		vc_mgmt[i].notif_pending = ATOMIC_INITIALIZER(0);
	}
}

void send_event(struct event_msg *ev_msg, unsigned ev_type, int vcoreid)
{
	struct pvc_event_msg *m = parlib_malloc(sizeof(struct pvc_event_msg));
	m->ev_msg = ev_msg;
	m->ev_type = ev_type;
	spinlock_lock(&(vc_mgmt[vcoreid].evq_lock));
	STAILQ_INSERT_TAIL(&(vc_mgmt[vcoreid].evq), m, next);
	spinlock_unlock(&(vc_mgmt[vcoreid].evq_lock));
	atomic_set(&vc_mgmt[vcoreid].notif_pending, 1);
	if (atomic_read(&vc_mgmt[vcoreid].notifs_enabled)) {
		atomic_set(&vc_mgmt[vcoreid].notif_pending, 0);
		vcore_signal(vcoreid);
	}
}

void handle_events()
{
	struct pvc_event_msg *m;
	int vcoreid = vcore_id();
	spinlock_lock(&(vc_mgmt[vcoreid].evq_lock));
	while((m = STAILQ_FIRST(&(vc_mgmt[vcoreid].evq)))) {
		STAILQ_REMOVE_HEAD(&(vc_mgmt[vcoreid].evq), next);
		spinlock_unlock(&(vc_mgmt[vcoreid].evq_lock));

		handle_event_t handler = ev_handlers[m->ev_type];
		handler(m->ev_msg, m->ev_type);
		free(m);

		spinlock_lock(&(vc_mgmt[vcoreid].evq_lock));
	}
	spinlock_unlock(&(vc_mgmt[vcoreid].evq_lock));
}

/* Enables notifs, and deals with missed notifs by self notifying.  This should
 * be rare, so the syscall overhead isn't a big deal. */
void enable_notifs(uint32_t vcoreid)
{
	if (atomic_swap(&vc_mgmt[vcoreid].notif_pending, 0) == 1)
		vcore_signal(vcoreid);
	atomic_set(&vc_mgmt[vcoreid].notifs_enabled, 1);
}

void disable_notifs(uint32_t vcoreid)
{
	atomic_set(&vc_mgmt[vcoreid].notifs_enabled, 0);
}

#undef event_lib_init
#undef send_event
#undef handle_events
#undef enable_notifs
#undef disable_notifs
EXPORT_ALIAS(INTERNAL(event_lib_init), event_lib_init)
EXPORT_ALIAS(INTERNAL(send_event), send_event)
EXPORT_ALIAS(INTERNAL(handle_events), handle_events)
EXPORT_ALIAS(INTERNAL(enable_notifs), enable_notifs)
EXPORT_ALIAS(INTERNAL(disable_notifs), disable_notifs)
