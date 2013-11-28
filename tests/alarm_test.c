#include "alarm.h"
#include "internal/time.h"
#include <stdio.h>

volatile int progress = 0;

void cb(struct alarm_waiter* awaiter) {
  printf("Hi, the current time is %f for %p\n", time_usec() * 1e-6, awaiter);
  __sync_fetch_and_add(&progress, 1);
}

int main() {
  struct alarm_waiter a, b;
  init_awaiter(&a, cb);
  init_awaiter(&b, cb);

  printf("Hi, the current time is %f\n", time_usec() * 1e-6);

  set_awaiter_rel(&a, 1000000);
  set_alarm(&a);
  while (progress < 1);

  set_awaiter_rel(&a, 100000);
  set_awaiter_rel(&b, 50000);
  set_alarm(&a);
  set_alarm(&b);
  while (progress < 3);

  return 0;
}
