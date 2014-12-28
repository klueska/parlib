#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "internal/parlib.h"
#include "internal/time.h"
#include "internal/pthread_pool.h"
#include "alarm.h"
#include "spinlock.h"
#include "export.h"

static void *__waiting_thread(void *arg)
{
	uint64_t wakeup_time, now;
	struct alarm_waiter *waiter = (struct alarm_waiter*)arg;
	spinlock_lock(&waiter->lock);
	do {
		wakeup_time = waiter->wakeup_time;
		now = time_usec();
		if (wakeup_time < now)
			break;
		spinlock_unlock(&waiter->lock);
		usleep(wakeup_time-now);
		spinlock_lock(&waiter->lock);
	} while(waiter->wakeup_time != wakeup_time);
	waiter->done = true;
	spinlock_unlock(&waiter->lock);

	if (!waiter->unset) {
		struct event_msg *ev_msg = parlib_malloc(sizeof(struct event_msg));
		ev_msg->ev_arg3 = waiter;
		send_event(ev_msg, EV_ALARM, waiter->vcoreid);
	}
	return NULL;
}

static void init_alarm_service(void)
{
	void handler(struct event_msg *ev_msg, unsigned int ev_type)
	{
		assert(in_vcore_context());
		assert(ev_msg);
		struct alarm_waiter *waiter = (struct alarm_waiter*)ev_msg->ev_arg3;
		free(ev_msg);
		waiter->func(waiter);
	}
	ev_handlers[EV_ALARM] = handler;
}

void EXPORT_SYMBOL init_awaiter(struct alarm_waiter *waiter,
                                void (*func) (struct alarm_waiter *))
{
	run_once_racy(init_alarm_service());
	waiter->func = func;
	waiter->wakeup_time = 0;
	waiter->unset = false;
	waiter->done = false;
	waiter->vcoreid = vcore_id();
	spinlock_init(&waiter->lock);
}

void EXPORT_SYMBOL set_awaiter_rel(struct alarm_waiter *waiter, uint64_t usleep)
{
	uint64_t now = time_usec();
	spinlock_lock(&waiter->lock);
	waiter->wakeup_time = now + usleep;
	spinlock_unlock(&waiter->lock);
}

void EXPORT_SYMBOL set_awaiter_inc(struct alarm_waiter *waiter, uint64_t usleep)
{
	assert(waiter->wakeup_time);
	spinlock_lock(&waiter->lock);
	waiter->wakeup_time += usleep;
	spinlock_unlock(&waiter->lock);
}

void EXPORT_SYMBOL set_alarm(struct alarm_waiter *waiter)
{
	assert(!waiter->unset);
	pooled_pthread_start(__waiting_thread, waiter);	
}

bool EXPORT_SYMBOL unset_alarm(struct alarm_waiter *waiter)
{
	spinlock_lock(&waiter->lock);
	if (!waiter->done) {
		waiter->unset = true;
	}
	spinlock_unlock(&waiter->lock);
	return waiter->unset;
}

