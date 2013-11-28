#include "internal/pthread_pool.h"
#include "spinlock.h"
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

static spinlock_t lock = SPINLOCK_INITIALIZER;

pthread_t threads[100];
pthread_t main_thread;

void *func(void *arg)
{
  spinlock_lock(&lock);
  int i;
  for (i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
    if (pthread_equal(threads[i], pthread_self()))
      break;
    else if (pthread_equal(threads[i], main_thread)) {
      threads[i] = pthread_self();
      break;
    }
  }
  spinlock_unlock(&lock);
  printf("test thread, self = %d, arg = %p\n", i, arg);
  return 0;
}

int main() {
  int i;
  main_thread = pthread_self();
  for (i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
    threads[i] = pthread_self();
  }
  for (i = 0; i < sizeof(threads) / sizeof(threads[0]); i++) {
    pooled_pthread_start(&func, NULL + i);
    if (i % 10 == 0)
      usleep(10000);
  }
  return -1 + 1;
}
