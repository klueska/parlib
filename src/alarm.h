#ifndef PARLIB_ALARM_H
#define PARLIB_ALARM_H

#include <stdint.h>
#include <stdbool.h>
#include "spinlock.h"

/* Specifc waiter, per alarm */
struct alarm_waiter {
    void     (*func) (struct alarm_waiter *waiter);
    uint64_t wakeup_time; /* in usec */  
    bool     unset;
    bool     done;
    void     *data;
    spinlock_t lock;
};

void init_awaiter(struct alarm_waiter *waiter,
                  void (*func) (struct alarm_waiter *));
/* Sets the time an awaiter goes off */
void set_awaiter_rel(struct alarm_waiter *waiter, uint64_t usleep);
void set_awaiter_inc(struct alarm_waiter *waiter, uint64_t usleep);
/* Arms/disarms the alarm */
void set_alarm(struct alarm_waiter *waiter);
bool unset_alarm(struct alarm_waiter *waiter);

// These are defined in akaros so we may need to find a way to implement them...
//void set_awaiter_abs(struct alarm_waiter *waiter, uint64_t abs_time);
//void reset_alarm_abs(struct alarm_waiter *waiter, uint64_t abs_time); 

#endif // PARLIB_ALARM_H
